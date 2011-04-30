/**\file hu_log.h
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2005-2009 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2005-2011 Daniel Swanson <danij@dengine.net>
 *\author Copyright © 1993-1996 by id Software, Inc.
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
 * Game message logging and display.
 */

#ifndef LIBCOMMON_HUD_LOG_H
#define LIBCOMMON_HUD_LOG_H

#if __JDOOM__
#  include "jdoom.h"
#elif __JDOOM64__
# include "jdoom64.h"
#elif __JHERETIC__
#  include "jheretic.h"
#elif __JHEXEN__
#  include "jhexen.h"
#endif

/**
 * @defgroup logMessageFlags  Log Message Flags.
 * @{
 */
#define LMF_NOHIDE          (0x1) /// Message will always be displayed (cannot be hidden by the player).
#define LMF_YELLOW          (0x2) /// Prepend the "yellow" paramater string to given message.
/**@}*/

/// To be called to register the console commands and variables of this module.
void Hu_LogRegister(void);

/// To be called to initialize this module for use by local @a player
void Hu_LogStart(int player);

/// To be called to shutdown this module. All logs for all players will be destroyed.
void Hu_LogShutdown(void);

/**
 * Post a message to the specified player's log.
 *
 * @param player  Player (local) number whose log to post to.
 * @param flags  @see logMessageFlags
 * @param msg  Message text to be posted.
 */
void Hu_LogPost(int player, byte flags, const char* msg);

/**
 * Rewind the message log of the specified player, making the last few messages
 * visible once again.
 *
 * @param player  Local player number whose message log to refresh.
 */
void Hu_LogRefresh(int player);

/**
 * Empty the message log of the specified player.
 *
 * @param player  Local player number whose message log to empty.
 */
void Hu_LogEmpty(int player);

/**
 * Draw the message log of the specified player.
 *
 * @param player  Local player number whose message log to draw.
 */
void Hu_LogDrawer(int player, float textAlpha, float iconAlpha, int* drawnWidth, int* drawnHeight);

/**
 * Process gametic for all players with an active message log.
 */
void Hu_LogTicker(void);

#endif /* LIBCOMMON_HUD_LOG_H */

/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2005-2009 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2005-2009 Daniel Swanson <danij@dengine.net>
 *\author Copyright © 1993-1996 by id Software, Inc.
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
 * hu_chat.h:
 */

#ifndef __HUD_CHAT_H__
#define __HUD_CHAT_H__

#if __JDOOM__
#  include "jdoom.h"
#elif __JDOOM64__
# include "jdoom64.h"
#elif __JHERETIC__
#  include "jheretic.h"
#elif __JHEXEN__
#  include "jhexen.h"
#endif

void            Chat_Register(void);
void            Chat_Init(void);

void            Chat_Start(void);
void            Chat_Open(int player, boolean open);
boolean         Chat_IsActive(int player);
boolean         Chat_Responder(event_t* ev);

void            Chat_Drawer(int player, float textAlpha, float iconAlpha, int* drawnWidth, int* drawnHeight);
#endif
