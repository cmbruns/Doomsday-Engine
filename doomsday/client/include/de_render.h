/**
 * @file de_render.h
 * Rendering subsystem. @ingroup render
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

#ifndef DOOMSDAY_CLIENT_RENDERER
#define DOOMSDAY_CLIENT_RENDERER

#include "render/r_main.h"

#ifdef __CLIENT__
#include "render/lightgrid.h"
#include "render/projector.h"
#include "render/r_draw.h"
#include "render/r_things.h"
#include "render/rend_clip.h"
#include "render/rend_halo.h"
#include "render/rend_particle.h"
#include "render/rend_main.h"
#include "render/rend_model.h"
#include "render/rend_shadow.h"
#include "render/rend_fakeradio.h"
#include "render/rend_font.h"
#include "render/rend_dynlight.h"
#include "render/rendpoly.h"
#include "render/sky.h"
#include "render/sprite.h"
#include "render/vignette.h"
#include "render/vissprite.h"
#include "render/vlight.h"
#endif

#include "r_util.h"

#endif /* DOOMSDAY_CLIENT_RENDERER */
