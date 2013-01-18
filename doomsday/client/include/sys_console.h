/**\file
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2009-2013 Daniel Swanson <danij@dengine.net>
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

/** @todo This needs further refactoring to fit more nicely into the rest
    of the window management. */

#ifndef LIBDENG_SYSTEM_CONSOLE_H
#define LIBDENG_SYSTEM_CONSOLE_H

struct consolewindow_s; // opaque type

#include "ui/window.h"
#include "ui/sys_input.h"

#ifdef __cplusplus
extern "C" {
#endif

Window* Sys_ConInit(const char* title);

void Sys_ConShutdown(Window *window);

void ConsoleWindow_SetTitle(const Window *window, const char* title);

/**
 * @param flags  @ref consolePrintFlags
 */
void Sys_ConPrint(uint idx, const char* text, int flags);

/**
 * Set the command line display of the specified console window.
 *
 * @param idx  Console window identifier.
 * @param text  Text string to copy.
 * @param cursorPos  Position to set the cursor on the command line.
 * @param flags  @ref consoleCommandlineFlags
 */
void Sys_SetConWindowCmdLine(uint idx, const char* text, unsigned int cursorPos, int flags);

size_t I_GetConsoleKeyEvents(keyevent_t *evbuf, size_t bufsize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif