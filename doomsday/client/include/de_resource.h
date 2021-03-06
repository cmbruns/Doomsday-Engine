/** @file de_resource.h Resource Subsystem.
 * @ingroup resource
 *
 * @author Copyright &copy; 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @author Copyright &copy; 2006-2013 Daniel Swanson <danij@dengine.net>
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

#ifndef DENG_RESOURCE_SUBSYSTEM_H
#define DENG_RESOURCE_SUBSYSTEM_H

#include "resourceclass.h"

#include "resource/animgroups.h"
#include "resource/colorpalettes.h"
#include "resource/font.h"
#include "resource/fonts.h"
#include "resource/r_data.h"
#include "Materials"
#include "resource/models.h"
#include "resource/compositetexture.h"
#include "resource/rawtexture.h"
#include "resource/sprites.h"
#include "Textures"

#ifdef __CLIENT__
#  include "resource/bitmapfont.h"
#  include "resource/materialsnapshot.h"
#  include "resource/materialvariantspec.h"
#endif

#ifdef __cplusplus
#  include "resource/patch.h"
#  include "resource/texturemanifest.h"
#  include "resource/wad.h"
#  include "resource/zip.h"
#endif

#include "api_resource.h"

#endif // DENG_RESOURCE_SUBSYSTEM_H
