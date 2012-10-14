/**
 * @file abstractfile.cpp
 *
 * Abstract base for all classes which represent loaded files.
 *
 * @ingroup fs
 *
 * @author Copyright &copy; 2003-2012 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @author Copyright &copy; 2005-2012 Daniel Swanson <danij@dengine.net>
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
#include "de_filesys.h"

#include "abstractfile.h"

namespace de {

AbstractFile::AbstractFile(filetype_t _type, char const* _path, DFile& file, LumpInfo const& _info)
    : file(&file), type_(_type), flags(DefaultFlags)
{
    // Used to favor newer files when duplicates are pruned.
    /// @todo Does not belong at this level. Load order should be determined
    ///       at file system level. -ds
    static uint fileCounter = 0;
    DENG2_ASSERT(VALID_FILETYPE(_type));
    order = fileCounter++;

    Str_Init(&path_); Str_Set(&path_, _path);
    info_ = _info;
}

AbstractFile::~AbstractFile()
{
    App_FileSystem()->releaseFile(*this);
    Str_Free(&path_);
    if(file) delete file;
}

filetype_t AbstractFile::type() const
{
    return type_;
}

LumpInfo const& AbstractFile::info() const
{
    return info_;
}

bool AbstractFile::isContained() const
{
    return !!info_.container;
}

AbstractFile& AbstractFile::container() const
{
    if(!info_.container) throw de::Error("AbstractFile::container", QString("%s is not contained").arg(Str_Text(path())));
    return *info_.container;
}

de::DFile& de::AbstractFile::handle()
{
    return *file;
}

ddstring_t const* AbstractFile::path() const
{
    return &path_;
}

uint AbstractFile::loadOrderIndex() const
{
    return order;
}

bool AbstractFile::hasStartup() const
{
    return flags.testFlag(Startup);
}

AbstractFile& AbstractFile::setStartup(bool yes)
{
    if(yes) flags |= Startup;
    else    flags &= ~Startup;
    return *this;
}

bool AbstractFile::hasCustom() const
{
    return flags.testFlag(Custom);
}

AbstractFile& AbstractFile::setCustom(bool yes)
{
    if(yes) flags |= Custom;
    else    flags &= ~Custom;
    return *this;
}

} // namespace de