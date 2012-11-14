/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2004-2012 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2009-2012 Daniel Swanson <danij@dengine.net>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/**
 * File Path Processing.
 */

#ifndef LIBDENG_FILESYS_PATH_H
#define LIBDENG_FILESYS_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert the given path to an absolute path.
 */
char* _fullpath(char* full, const char* original, int len);

void _splitpath(const char* path, char* drive, char* dir, char* name, char* ext);

#ifdef __cplusplus
}
#endif

#endif /* LIBDENG_FILESYS_PATH_H */