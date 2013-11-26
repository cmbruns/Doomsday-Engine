/** @file bank.cpp  Abstract data bank with multi-tiered caching.
 *
 * @authors Copyright (c) 2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 *
 * @par License
 * GPL: http://www.gnu.org/licenses/gpl.html
 *
 * <small>This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. This program is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the GNU
 * General Public License along with this program; if not, see:
 * http://www.gnu.org/licenses</small> 
 */

#include "de/Bank"
#include "de/Folder"
#include "de/App"
#include "de/Loop"
#include "de/FS"
#include "de/PathTree"
#include "de/WaitableFIFO"
#include "de/Task"
#include "de/TaskPool"
#include "de/Log"
#include "de/Writer"
#include "de/Reader"
#include "de/math.h"

#include <QThread>
#include <QList>

namespace de {

namespace internal {

/**
 * Cache of objects of type ItemType. Does not own the objects or the data,
 * merely guides operations on them.
 */
template <typename ItemType>
class Cache : public Lockable
{
public:
    enum Format {
        Source,     ///< Data is present only as source information.
        Object,     ///< Data is present as a fully usable object in memory.
        Serialized  ///< Data is present as a block of serialized bytes.
    };

    static char const *formatAsText(Format f) {
        switch(f) {
        case Source:     return "Source";     break;
        case Object:     return "Object";     break;
        case Serialized: return "Serialized"; break;
        }
        return "";
    }

    typedef QSet<ItemType *> Items;

public:
    Cache(Format format)
        : _format(format),
          _maxBytes(Bank::Unlimited),
          _currentBytes(0),
          _maxItems(Bank::Unlimited) {}

    virtual ~Cache() {}

    void setMaxBytes(dint64 m) { _maxBytes = m; }
    void setMaxItems(int m) { _maxItems = m; }

    Format format() const { return _format; }
    dint64 maxBytes() const { return _maxBytes; }
    dint64 byteCount() const { return _currentBytes; }
    int maxItems() const { return _maxItems; }
    int itemCount() const { return _items.size(); }

    virtual void add(ItemType &data) {
        _items.insert(&data);
    }

    virtual void remove(ItemType &data) {
        _items.remove(&data);
    }

    virtual void clear() {
        DENG2_GUARD(this);
        _items.clear();
        _currentBytes = 0;
    }

    Items const &items() const { return _items; }

protected:
    void addBytes(dint64 bytes) {
        _currentBytes = de::max(dint64(0), _currentBytes + bytes);
    }

private:
    Format _format;
    dint64 _maxBytes;
    dint64 _currentBytes;
    int _maxItems;
    Items _items;
};

} // namespace internal

DENG2_PIMPL(Bank),
DENG2_OBSERVES(Loop, Iteration) // notifications from other threads sent via main Loop
{
    /**
     * Data item. Has ownership of the in-memory cached data and the source
     * information. Lockable because may be accessed from the worker thread in
     * addition to caller thread(s).
     */
    struct Data : public PathTree::Node, public Waitable, public Lockable
    {
        typedef internal::Cache<Data> Cache;

        Bank *bank;                     ///< Bank that owns the data.
        std::auto_ptr<IData> data;      ///< Non-NULL for in-memory items.
        std::auto_ptr<ISource> source;  ///< Always required.
        IByteArray *serial;             ///< Serialized representation (if one is present; not owned).
        Cache *cache;                   ///< Current cache for the data (never NULL).
        Time accessedAt;

        Data(PathTree::NodeArgs const &args)
            : Node(args),
              bank(0),
              serial(0),
              cache(0),
              accessedAt(Time::invalidTime())
        {}

        void clearData()
        {
            DENG2_GUARD(this);

            if(data.get())
            {
                LOG_VERBOSE("Item \"%s\" data cleared from memory (%i bytes)")
                        << path('.') << data->sizeInMemory();
                data->aboutToUnload();
                data.reset();
            }
        }

        void setData(IData *newData)
        {
            DENG2_GUARD(this);
            DENG2_ASSERT(newData != 0);

            data.reset(newData);
            accessedAt = Time();
            bank->d->notify(Notification(Notification::Loaded, path('.')));
        }

        /// Load the item into memory from its current cache.
        void load()
        {
            DENG2_GUARD(this);
            DENG2_ASSERT(cache != 0);

            switch(cache->format())
            {
            case Cache::Source:
                loadFromSource();
                break;

            case Cache::Serialized:
                loadFromSerialized();
                break;

            case Cache::Object:
                // No need to do anything, already loaded.
                break;
            }
        }

        void loadFromSource()
        {
            DENG2_ASSERT(source.get() != 0);

            Time startedAt;

            // Ask the concrete bank implementation to load the data for
            // us. This may take an unspecified amount of time.
            QScopedPointer<IData> loaded(bank->loadFromSource(*source));

            LOG_TRACE("Loaded \"%s\" from source in %.2f seconds") << path('.') << startedAt.since();

            if(loaded.data())
            {
                // Put the loaded data into the memory cache.
                setData(loaded.take());
            }
        }

        bool isValidSerialTime(Time const &serialTime) const
        {
            return (!source->modifiedAt().isValid() ||
                    source->modifiedAt() == serialTime);
        }

        void loadFromSerialized()
        {
            DENG2_ASSERT(serial != 0);

            try
            {
                Time startedAt;

                Time timestamp(Time::invalidTime());
                Reader reader(*serial);
                reader.withHeader() >> timestamp;

                if(isValidSerialTime(timestamp))
                {
                    QScopedPointer<IData> blank(bank->newData());
                    reader >> *blank->asSerializable();
                    setData(blank.take());
                    LOG_TRACE("Deserialized \"%s\" in %.2f seconds") << path('.') << startedAt.since();
                    return; // Done!
                }
                // We cannot use this.
            }
            catch(Error const &er)
            {
                LOG_WARNING("Failed to deserialize \"%s\":\n") << path('.') << er.asText();
            }

            // Fallback option.
            loadFromSource();
        }

        void serialize(Folder &folder)
        {
            DENG2_GUARD(this);

            if(serial)
            {
                // Already serialized.
                return;
            }

            DENG2_ASSERT(source.get() != 0);

            if(!data.get())
            {
                // We must have the object in memory first.
                loadFromSource();
            }

            DENG2_ASSERT(data->asSerializable() != 0);

            try
            {
                // Make sure the correct folder exists.
                Folder &containingFolder = folder.fileSystem()
                        .makeFolder(folder.path() / path().toString().fileNamePath());

                // Source timestamp is included in the serialization
                // to check later whether the data is still fresh.
                serial = dynamic_cast<IByteArray *>(
                            &containingFolder.newFile(name(), Folder::ReplaceExisting));
                DENG2_ASSERT(serial != 0);

                Writer(*serial).withHeader()
                        << source->modifiedAt()
                        << *data->asSerializable();
            }
            catch(...)
            {
                serial = 0;
                throw;
            }
        }

        void clearSerialized()
        {
            DENG2_GUARD(this);

            serial = 0;
        }

        void changeCache(Cache &toCache)
        {
            DENG2_GUARD(this);
            DENG2_ASSERT(cache != 0);

            if(cache != &toCache)
            {
                Cache &fromCache = *cache;
                toCache.add(*this);
                fromCache.remove(*this);
                cache = &toCache;

                // Externally we use dotted paths.
                Path const itemPath = path('.');

                LOG_TRACE("Item \"%s\" moved to %s cache")
                        << itemPath << Cache::formatAsText(toCache.format());

                bank->d->notify(Notification(itemPath, toCache));
            }
        }
    };

    typedef PathTreeT<Data> DataTree;
    typedef internal::Cache<Data> DataCache;

    /**
     * Dummy cache representing objects in the source data.
     */
    struct SourceCache : public DataCache
    {
        SourceCache() : DataCache(Source) {}
    };

    /**
     * Hot storage containing data items serialized into files. The goal is to
     * allow quick recovery of data into memory. May be disabled in a Bank.
     */
    class SerializedCache : public DataCache
    {
    public:
        SerializedCache() : DataCache(Serialized), _folder(0) {}

        void add(Data &item)
        {
            DENG2_GUARD(this);

            DENG2_ASSERT(_folder != 0);
            item.serialize(*_folder);
            addBytes(item.serial->size());
            DataCache::add(item);
        }

        void remove(Data &item)
        {
            DENG2_GUARD(this);

            addBytes(-dint64(item.serial->size()));
            item.clearSerialized();
            DataCache::remove(item);
        }

        void setLocation(String const &location)
        {
            DENG2_ASSERT(!location.isEmpty());
            DENG2_GUARD(this);

            // Serialized "hot" data is kept here.
            _folder = &App::fileSystem().makeFolder(location);
        }

        Folder const &folder() const
        {
            DENG2_ASSERT(_folder != 0);
            return *_folder;
        }

        Folder &folder()
        {
            DENG2_ASSERT(_folder != 0);
            return *_folder;
        }

    private:
        Folder *_folder;
    };

    /**
     * Cache of objects in memory.
     */
    class ObjectCache : public DataCache
    {
    public:
        ObjectCache() : DataCache(Object)
        {}

        void add(Data &item)
        {
            // Acquire the object.
            item.load();

            DENG2_GUARD(this);

            DENG2_ASSERT(item.data.get() != 0);

            addBytes(item.data->sizeInMemory());
            DataCache::add(item);
        }

        void remove(Data &item)
        {
            DENG2_ASSERT(item.data.get() != 0);

            DENG2_GUARD(this);

            addBytes(-dint64(item.data->sizeInMemory()));
            item.clearData();
            DataCache::remove(item);
        }
    };

    /**
     * Operation on a data item (e.g., loading, serialization). Run by TaskPool
     * in a background thread.
     */
    class Job : public Task
    {
    public:
        enum Task {
            Load,
            Serialize,
            Unload
        };

    public:
        Job(Bank &bk, Task t, Path const &p = Path())
            : _bank(bk), _task(t), _path(p)
        {}

        void runTask()
        {
            LOG_AS("Bank::Job");

            switch(_task)
            {
            case Job::Load:
                doLoad();
                break;

            case Job::Serialize:
                doSerialize();
                break;

            case Job::Unload:
                doUnload();
                break;
            }
        }

        Data &item()
        {
            return _bank.d->items.find(_path, PathTree::MatchFull | PathTree::NoBranch);
        }

        void doLoad()
        {
            try
            {
                Data &it = item();
                it.changeCache(_bank.d->memoryCache);
            }
            catch(Error const &er)
            {
                LOG_WARNING("Failed to load \"%s\" from source:\n") << _path << er.asText();
            }
            // Ensure a blocking load completes.
            item().post();
        }

        void doSerialize()
        {
            try
            {
                DENG2_ASSERT(_bank.d->serialCache != 0);

                LOG_DEBUG("Serializing \"%s\"") << _path;
                item().changeCache(*_bank.d->serialCache);
            }
            catch(Error const &er)
            {
                LOG_WARNING("Failed to serialize \"%s\" to hot storage:\n")
                        << _path << er.asText();
            }
        }

        void doUnload()
        {
            try
            {
                LOG_DEBUG("Unloading \"%s\"") << _path;
                item().changeCache(_bank.d->sourceCache);
            }
            catch(Error const &er)
            {
                LOG_WARNING("Error when unloading \"%s\":\n")
                        << _path << er.asText();
            }
        }

    private:
        Bank &_bank;
        Task _task;
        Path _path;
    };

    /**
     * Notification about status changes of data in the tree.
     */
    struct Notification
    {
        enum Kind { CacheChanged, Loaded };

        Kind kind;
        Path path;
        DataCache *cache;

        Notification(Kind k, Path const &p)
            : kind(k), path(p), cache(0) {}

        Notification(Path const &p, DataCache &c)
            : kind(CacheChanged), path(p), cache(&c) {}
    };

    typedef FIFO<Notification> NotifyQueue;

    Flags flags;
    SourceCache sourceCache;
    ObjectCache memoryCache;
    SerializedCache *serialCache;
    DataTree items;
    TaskPool jobs;
    NotifyQueue notifications;

    Instance(Public *i, Flags const &flg)
        : Base(i),
          flags(flg),
          serialCache(0)
    {
        if(!flags.testFlag(DisableHotStorage))
        {
            serialCache = new SerializedCache;
        }
    }

    ~Instance()
    {
        Loop::appLoop().audienceForIteration -= this;
        destroySerialCache();
    }

    void destroySerialCache()
    {
        jobs.waitForDone();

        // Should we delete the actual files where the data has been kept?
        if(serialCache && flags.testFlag(ClearHotStorageWhenBankDestroyed))
        {
            Folder &folder = serialCache->folder();
            PathTree::FoundPaths paths;
            items.findAllPaths(paths, PathTree::NoBranch);
            DENG2_FOR_EACH(PathTree::FoundPaths, i, paths)
            {
                if(folder.has(*i))
                {
                    folder.removeFile(*i);
                }
            }
        }

        delete serialCache;
        serialCache = 0;
    }

    inline bool isThreaded() const
    {
        return flags.testFlag(BackgroundThread);
    }

    void beginJob(Job *job, Importance importance)
    {
        if(!isThreaded())
        {
            // Execute the job immediately.
            QScopedPointer<Job> j(job);
            j->runTask();
            performNotifications();
        }
        else
        {
            jobs.start(job, importance == AfterQueued?
                           TaskPool::LowPriority : TaskPool::HighPriority);
        }
    }

    void clear()
    {
        jobs.waitForDone();        

        items.clear();
        sourceCache.clear();
        memoryCache.clear();
        if(serialCache)
        {
            serialCache->clear();
        }
    }

    void setSerialLocation(String const &location)
    {
        if(location.isEmpty() || flags.testFlag(DisableHotStorage))
        {
            destroySerialCache();
        }
        else
        {
            if(!serialCache) serialCache = new SerializedCache;
            serialCache->setLocation(location);
        }
    }

    void putInBestCache(Data &item)
    {
        DENG2_ASSERT(item.cache == 0);

        // The source cache is always good.
        DataCache *best = &sourceCache;

        if(serialCache)
        {
            // Check if this item is already available in hot storage.
            IByteArray *array = serialCache->folder().tryLocate<IByteArray>(item.path());
            if(array)
            {
                Time hotTime;
                Reader(*array).withHeader() >> hotTime;

                if(item.isValidSerialTime(hotTime))
                {
                    LOG_VERBOSE("Found valid serialized copy of \"%s\"") << item.path('.');

                    item.serial = array;
                    best = serialCache;
                }
            }
        }

        item.cache = best;
        best->add(item);
    }

    void load(Path const &path, Importance importance)
    {       
        beginJob(new Job(self, Job::Load, path), importance);
    }

    void unload(Path const &path, CacheLevel toLevel)
    {
        if(toLevel < InMemory)
        {
            Job::Task const task = (toLevel == InHotStorage && serialCache?
                                    Job::Serialize : Job::Unload);
            beginJob(new Job(self, task, path), Immediately);
        }
    }

    void notify(Notification const &notif)
    {
        notifications.put(new Notification(notif));
        if(isThreaded())
        {
            Loop::appLoop().audienceForIteration += this;
        }
    }

    void loopIteration()
    {
        Loop::appLoop().audienceForIteration -= this;
        performNotifications();
    }

    void performNotifications()
    {
        forever
        {
            QScopedPointer<Notification> notif(notifications.take());
            if(!notif.data()) break;

            performNotification(*notif);
        }
    }

    void performNotification(Notification const &nt)
    {
        switch(nt.kind)
        {
        case Notification::Loaded:
            DENG2_FOR_PUBLIC_AUDIENCE(Load, i)
            {
                i->bankLoaded(nt.path);
            }
            break;

        case Notification::CacheChanged:
            DENG2_FOR_PUBLIC_AUDIENCE(CacheLevel, i)
            {
                DENG2_ASSERT(nt.cache != 0);

                i->bankCacheLevelChanged(nt.path,
                      nt.cache == &memoryCache? InMemory :
                      nt.cache == serialCache?  InHotStorage :
                                                InColdStorage);
            }
            break;
        }
    }
};

Bank::Bank(Flags const &flags, String const &hotStorageLocation)
    : d(new Instance(this, flags))
{
    d->setSerialLocation(hotStorageLocation);
}

Bank::~Bank()
{}

Bank::Flags Bank::flags() const
{
    return d->flags;
}

void Bank::setHotStorageCacheLocation(String const &location)
{
    d->setSerialLocation(location);
}

void Bank::setHotStorageSize(dint64 maxBytes)
{
    if(d->serialCache)
    {
        d->serialCache->setMaxBytes(maxBytes);
    }
}

void Bank::setMemoryCacheSize(dint64 maxBytes)
{
    d->memoryCache.setMaxBytes(maxBytes);
}

String Bank::hotStorageCacheLocation() const
{
    if(d->serialCache)
    {
        return d->serialCache->folder().path();
    }
    return "";
}

dint64 Bank::hotStorageSize() const
{
    if(d->serialCache)
    {
        return d->serialCache->maxBytes();
    }
    return 0;
}

dint64 Bank::memoryCacheSize() const
{
    return d->memoryCache.maxBytes();
}

void Bank::clear()
{
    d->clear();
}

void Bank::add(DotPath const &path, ISource *source)
{
    LOG_AS("Bank");

    QScopedPointer<ISource> src(source);
    Instance::Data &item = d->items.insert(path);

    DENG2_GUARD(item);

    item.bank = this;
    item.source.reset(src.take());

    d->putInBestCache(item);
}

void Bank::remove(DotPath const &path)
{
    d->items.remove(path, PathTree::NoBranch);
}

bool Bank::has(DotPath const &path) const
{
    return d->items.has(path);
}

dint Bank::allItems(Names &names) const
{
    names.clear();
    PathTree::FoundPaths paths;
    d->items.findAllPaths(paths, PathTree::NoBranch);
    DENG2_FOR_EACH(PathTree::FoundPaths, i, paths)
    {
        names.insert(*i);
    }
    return dint(names.size());
}

PathTree const &Bank::index() const
{
    return d->items;
}

void Bank::load(DotPath const &path, Importance importance)
{
    d->load(path, importance);
}

void Bank::loadAll()
{
    Names names;
    allItems(names);
    DENG2_FOR_EACH(Names, i, names)
    {
        load(*i, AfterQueued);
    }
}

Bank::IData &Bank::data(DotPath const &path) const
{
    LOG_AS("Bank");

    // First check if the item is already in memory.
    Instance::Data &item = d->items.find(path, PathTree::MatchFull | PathTree::NoBranch);
    DENG2_GUARD(item);

    // Mark it used.
    item.accessedAt = Time();

    if(item.data.get())
    {
        return *item.data;
    }

    // We'll have to request and wait.
    item.reset();
    item.unlock();

    LOG_TRACE("Loading \"%s\"...") << path;

    Time requestedAt;
    d->load(path, Immediately);
    item.wait();

    LOG_DEBUG("\"%s\" loaded (waited %.2f seconds)") << path << requestedAt.since();

    item.lock();
    if(!item.data.get())
    {
        throw LoadError("Bank::data", "Failed to load \"" + path + "\"");
    }
    return *item.data;
}

void Bank::unload(DotPath const &path, CacheLevel toLevel)
{
    d->unload(path, toLevel);
}

void Bank::unloadAll(CacheLevel maxLevel)
{
    if(maxLevel >= InMemory) return;

    Names names;
    allItems(names);
    DENG2_FOR_EACH(Names, i, names)
    {
        unload(*i, maxLevel);
    }
}

void Bank::clearFromCache(DotPath const &path)
{
    d->unload(path, InColdStorage);
}

void Bank::purge()
{
    /**
     * @todo Implement cache purging (and different purging strategies?).
     * Purge criteria can be age and cache level maximum limits.
     */
}

Bank::IData *Bank::newData()
{
    return 0;
}

} // namespace de
