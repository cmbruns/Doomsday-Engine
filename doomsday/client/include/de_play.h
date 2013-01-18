/**\file
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2005-2013 Daniel Swanson <danij@dengine.net>
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

/**
 * de_play.h: Game World Events (Playsim)
 */

#ifndef __DOOMSDAY_PLAYSIM__
#define __DOOMSDAY_PLAYSIM__

#include "api_thinker.h"
#ifdef __cplusplus
#  include "map/vertex.h"
#endif
#include "map/surface.h"
#include "map/sidedef.h"
#include "map/linedef.h"
#include "map/plane.h"
#include "map/hedge.h"
#include "map/bspleaf.h"
#include "map/bspnode.h"
#include "map/sector.h"
#include "map/polyobj.h"
#include "map/p_polyobjs.h"
#include "map/p_dmu.h"
#include "map/p_object.h"
#include "map/p_intercept.h"
#include "map/p_maputil.h"
#include "map/p_particle.h"
#include "map/p_sight.h"
#include "map/p_ticker.h"
#include "map/p_players.h"
#include "map/p_objlink.h"
#include "map/p_mapdata.h"
#include "map/p_maptypes.h"
#include "map/r_world.h"
#include "resource/material.h"
#include "ui/p_control.h"
#include "r_util.h"

#include "api_map.h"

#endif