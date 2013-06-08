/** @file entitydatabase.cpp Database of map entity property values. 
 * @ingroup map
 *
 * @authors Copyright &copy; 2007-2013 Daniel Swanson <danij@dengine.net>
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
 * General Public License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA</small>
 */

#include "de_base.h"
#include "map/p_mapdata.h"
#include "map/propertyvalue.h"

#include <map>
#include <de/Error>
#include <de/Log>

struct entitydatabase_s
{
private:
    // An entity is a set of one or more properties.
    // Key is the unique identifier of said property in the MapEntityPropertyDef it is derived from.
    typedef std::map<int, PropertyValue*> Entity;
    // Entities are stored in a set, each associated with a unique map element index.
    typedef std::map<int, Entity> Entities;
    // Entities are grouped in sets by their unique identifier.
    typedef std::map<int, Entities> EntitySet;

public:
    ~entitydatabase_s()
    {
        DENG2_FOR_EACH(EntitySet, setIt, entitySets)
        DENG2_FOR_EACH(Entities, entityIt, setIt->second)
        DENG2_FOR_EACH(Entity, propIt, entityIt->second)
        {
            delete propIt->second;
        }
    }

    /// @return Total number of entities by definition @a entityDef.
    uint entityCount(MapEntityDef const* entityDef)
    {
        DENG2_ASSERT(entityDef);
        Entities* set = entities(entityDef->id);
        return set->size();
    }

    /// @return @c true= An entity by definition @a entityDef and @a elementIndex is known/present.
    bool hasEntity(MapEntityDef const* entityDef, int elementIndex)
    {
        DENG2_ASSERT(entityDef);
        Entities* set = entities(entityDef->id);
        return entityByElementIndex(*set, elementIndex, false /*do not create*/) != 0;
    }

    /**
     * Lookup a known entity element property value in the database.
     *
     * @throws de::Error If @a elementIndex is out-of-range.
     *
     * @param def           Definition of the property to lookup an element value for.
     * @param elementIndex  Unique element index of the value to lookup.
     *
     * @return The found PropertyValue.
     */
    PropertyValue const& property(MapEntityPropertyDef const* def, int elementIndex)
    {
        DENG2_ASSERT(def);
        Entities* set = entities(def->entity->id);
        Entity* entity = entityByElementIndex(*set, elementIndex, false /*do not create*/);
        if(!entity) throw de::Error("property", QString("There is no element %1 of type %2")
                                                    .arg(elementIndex).arg(Str_Text(P_NameForMapEntityDef(def->entity))));

        Entity::const_iterator found = entity->find(def->id);
        DENG2_ASSERT(found != entity->end()); // Sanity check.
        return *found->second;
    }

    /**
     * Replace/add a value for a known entity element property to the database.
     *
     * @param def           Definition of the property to add an element value for.
     * @param elementIndex  Unique element index for the value.
     * @param value         The new PropertyValue. Ownership passes to this database.
     */
    void setProperty(MapEntityPropertyDef const* def, int elementIndex, PropertyValue* value)
    {
        DENG2_ASSERT(def);
        Entities* set = entities(def->entity->id);
        Entity* entity = entityByElementIndex(*set, elementIndex, true);
        if(!entity) throw de::Error("setProperty", "Failed adding new entity element record");

        // Do we already have a record for this?
        Entity::iterator found = entity->find(def->id);
        if(found != entity->end())
        {
            PropertyValue** adr = &found->second;
            delete *adr;
            *adr = value;
            return;
        }

        // Add a new record.
        entity->insert(std::pair<int, PropertyValue*>(def->id, value));
    }

private:
    /// Lookup the set in which entities with the unique identifier @a entityId are stored.
    Entities* entities(int entityId)
    {
        std::pair<EntitySet::iterator, bool> result;
        result = entitySets.insert(std::pair<int, Entities>(entityId, Entities()));
        return &result.first->second;
    }

    /// Lookup an entity in @a set by its unique @a elementIndex.
    Entity* entityByElementIndex(Entities& set, int elementIndex, bool canCreate)
    {
        // Do we already have a record for this entity?
        Entities::iterator found = set.find(elementIndex);
        if(found != set.end()) return &found->second;

        if(!canCreate) return 0;

        // A new entity.
        std::pair<Entities::iterator, bool> result;
        result = set.insert(std::pair<int, Entity>(elementIndex, Entity()));
        return &result.first->second;
    }

    EntitySet entitySets;
};

EntityDatabase* EntityDatabase_New(void)
{
    void* region = Z_Calloc(sizeof(entitydatabase_s), PU_MAPSTATIC, 0);
    return new (region) entitydatabase_s();
}

void EntityDatabase_Delete(EntityDatabase* db)
{
    if(!db) return;
    db->~entitydatabase_s();
    Z_Free(db);
}

uint EntityDatabase_EntityCount(EntityDatabase* db, MapEntityDef* entityDef)
{
    DENG2_ASSERT(db);
    return db->entityCount(entityDef);
}

boolean EntityDatabase_HasEntity(EntityDatabase* db, MapEntityDef* entityDef, int elementIndex)
{
    DENG2_ASSERT(db);
    return CPP_BOOL(db->hasEntity(entityDef, elementIndex));
}

PropertyValue const* EntityDatabase_Property(EntityDatabase* db,
    struct mapentitypropertydef_s* propertyDef, int elementIndex)
{
    DENG2_ASSERT(db);
    return &db->property(propertyDef, elementIndex);
}

boolean EntityDatabase_SetProperty(EntityDatabase* db, MapEntityPropertyDef* propertyDef,
    int elementIndex, valuetype_t valueType, void* valueAdr)
{
    DENG2_ASSERT(db);
    db->setProperty(propertyDef, elementIndex, BuildPropertyValue(valueType, valueAdr));
    return true;
}