/**\file dd_bitmapfont.h
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 1999-2011 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2006-2011 Daniel Swanson <danij@dengine.net>
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
 * Bitmap Fonts.
 */

#ifndef LIBDENG_API_BITMAPFONT_H
#define LIBDENG_API_BITMAPFONT_H

#include "dd_uri.h"

typedef uint32_t fontid_t;

/**
 * Find the associated id for a font named @a name, if it cannot be found
 * a fatal error is produced.
 *
 * @param name          Name of the font to look up.
 * @return  Unique id of the found font.
 */
fontid_t FR_FontIdForName(const char* name);

/**
 * Same as FR_SafeFontIdForName except does not produce a fatal error if the
 * specified font is not found.
 *
 * @param name          Name of the font to look up.
 * @return  Unique id of the font if found else @c 0
 */
fontid_t FR_SafeFontIdForName(const char* name);

void FR_ResetTypeInTimer(void);

/**
 * @defGroup drawTextFlags Draw Text Flags
 */
/*@{*/
#define DTF_NO_TYPEIN           0x0001
#define DTF_NO_SHADOW           0x0002
#define DTF_NO_GLITTER          0x0004

#define DTF_NO_EFFECTS          (DTF_NO_TYPEIN|DTF_NO_SHADOW|DTF_NO_GLITTER)
#define DTF_ONLY_SHADOW         (DTF_NO_TYPEIN|DTF_NO_GLITTER)
/*@}*/

#define FR_MAX_ATTRIB_STACK_DEPTH   (4)

/// Change the current font.
void FR_SetFont(fontid_t font);

/// Push the attribute stack.
void FR_PushAttrib(void);

/// Pop the attribute stack.
void FR_PopAttrib(void);

/// Load the default attributes at the current stack depth.
void FR_LoadDefaultAttrib(void);

void FR_SetTracking(int tracking);

void FR_SetShadowOffset(int offsetX, int offsetY);

void FR_SetShadowStrength(float value);

void FR_SetGlitterStrength(float value);

/// @return  Unique identifier associated with the current font.
fontid_t FR_GetCurrentId(void);

/// Current tracking.
int FR_Tracking(void);

/**
 * Text: A block of possibly formatted and/or multi-line text.
 */
void FR_DrawText(const char* string, int x, int y, fontid_t defFont, int defAlignFlags, short defFlags, float defLeading, int defTracking, float defRed, float defGreen, float defBlue, float defAlpha, float defGlitter, float defShadow, boolean defCase);

// Utility routines:
void FR_TextDimensions(int* width, int* height, const char* string, fontid_t defFont);
int FR_TextWidth(const char* string, fontid_t defFont);
int FR_TextHeight(const char* string, fontid_t defFont);

/**
 * Text fragments: A single line of unformatted text.
 */
void FR_DrawTextFragment(const char* string, int x, int y);
void FR_DrawTextFragment2(const char* string, int x, int y, int alignFlags, short flags);

// Utility routines:
void FR_TextFragmentDimensions(int* width, int* height, const char* string);
int FR_TextFragmentWidth(const char* string);
int FR_TextFragmentHeight(const char* string);

/**
 * Single character.
 */
void FR_DrawChar(unsigned char ch, int x, int y);
void FR_DrawChar2(unsigned char ch, int x, int y, int alignFlags, short flags);

// Utility routines:
void FR_CharDimensions(int* width, int* height, unsigned char ch);
int FR_CharWidth(unsigned char ch);
int FR_CharHeight(unsigned char ch);

#endif /* LIBDENG_API_BITMAPFONT_H */
