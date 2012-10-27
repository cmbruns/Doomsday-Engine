/*
 * The Doomsday Engine Project -- libdeng2
 *
 * Copyright (c) 2009-2012 Jaakko Keränen <jaakko.keranen@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBDENG2_LIBRARYFILE_H
#define LIBDENG2_LIBRARYFILE_H

#include "../File"

namespace de
{
    class Library;
    
    /**
     * Provides a way to load a shared library. The Library will be loaded
     * automatically when someone attempts to use it. Unloading the library occurs when
     * the LibraryFile instance is deleted, or when unload() is called.
     *
     * @see Library
     *
     * @ingroup fs
     */
    class DENG2_PUBLIC LibraryFile : public File
    {
    public:
        /// Attempted to load a shared library from a source file with unsupported type.
        /// @ingroup errors
        DENG2_ERROR(UnsupportedSourceError);
        
    public:
        /**
         * Constructs a new LibraryFile instance.
         *
         * @param source  Library file. Ownership transferred to LibraryFile.
         */
        LibraryFile(File* source);

        /**
         * When the LibraryFile is deleted the library is gets unloaded.
         */
        virtual ~LibraryFile();
        
        /**
         * Determines whether the library is loaded and ready for use.
         *
         * @return  @c true, if the library has been loaded.
         */
        bool loaded() const { return _library != 0; }
        
        /**
         * Provides access to the loaded library. Automatically attempts to load 
         * the library if it hasn't been loaded yet.
         *
         * @return  The library.
         */
        Library& library();
        
        /**
         * Unloads the library.
         */
        void clear();
        
        /**
         * Checks whether the name of the library file matches.
         *
         * @param nameAfterUnderscore  Part of the name following underscore.
         */
        bool hasUnderscoreName(const String& nameAfterUnderscore) const;

    public:
        /**
         * Determines whether a file appears suitable for use with LibraryFile.
         *
         * @param file  File whose content to recognize.
         */
        static bool recognize(const File& file);
        
    private:
        Library* _library;
    };
}

#endif /* LIBDENG2_LIBRARYFILE_H */