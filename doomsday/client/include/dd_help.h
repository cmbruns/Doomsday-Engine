/** @file dd_help.h Runtime help text strings.
 * @ingroup base
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2006-2013 Daniel Swanson <danij@dengine.net>
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

#ifndef LIBDENG_HELP_H
#define LIBDENG_HELP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void const *HelpId;

// Help string types.
enum {
    HST_DESCRIPTION,
    HST_CONSOLE_VARIABLE,
    HST_DEFAULT_VALUE,
    HST_INFO,
    NUM_HELPSTRING_TYPES
};

void DH_Register(void);

/**
 * Initializes the help string database. After which, attempts to read the
 * engine's own help string file.
 */
void DD_InitHelp(void);

/**
 * Attempts to read help strings from the game-specific help file.
 */
void DD_ReadGameHelp(void);

/**
 * Shuts down the help string database. Frees all storage and destroys
 * database itself.
 */
void DD_ShutdownHelp(void);

/**
 * Finds a node matching the ID. Use DH_GetString to read strings from it.
 *
 * @param id  Help node ID to be searched for.
 *
 * @return Pointer to help data, if matched; otherwise @c NULL.
 */
HelpId DH_Find(char const *id);

/**
 * Return a string from within the helpnode. Strings are stored internally
 * and indexed by their type (e.g. HST_DESCRIPTION).
 *
 * @param found  The helpnode to return the string from.
 * @param type   The string type (index) to look for within the node.
 *
 * @return Pointer to the found string; otherwise @c NULL. Note, may also
 * return @c NULL, if passed an invalid helpnode ptr OR the help string
 * database has not beeen initialized yet. The returned string is actually from
 * an AutoStr; it will only be valid until the next garbage recycling.
 */
char const *DH_GetString(HelpId found, int type);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LIBDENG_HELP_H */
