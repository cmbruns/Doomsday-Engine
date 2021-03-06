/*
 * The Doomsday Engine Project -- libdeng2
 *
 * Copyright (c) 2009-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
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

#include "de/Feed"

namespace de {

Feed::Feed()
{}

Feed::~Feed()
{}

File *Feed::newFile(String const &/*name*/)
{
    // By default feeds can't create files.
    return 0;
}

void Feed::removeFile(String const &/*name*/)
{}

Feed *Feed::newSubFeed(String const &/*name*/)
{
    // By default feeds can't create subfeeds.
    return 0;
}

} // namespace de
