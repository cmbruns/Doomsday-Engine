/** @file lineinfo.h BSP Builder Line info.
 *
 * Originally based on glBSP 2.24 (in turn, based on BSP 2.3)
 * @see http://sourceforge.net/projects/glbsp/
 *
 * @authors Copyright © 2007-2013 Daniel Swanson <danij@dengine.net>
 * @authors Copyright © 2000-2007 Andrew Apted <ajapted@gmail.com>
 * @authors Copyright © 1998-2000 Colin Reed <cph@moria.org.uk>
 * @authors Copyright © 1998-2000 Lee Killough <killough@rsn.hp.com>
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

#ifndef LIBDENG_BSP_LINEINFO
#define LIBDENG_BSP_LINEINFO

#include "map/gamemap.h"
#include "map/bsp/partitioner.h"

#include <QtGlobal>

namespace de {
namespace bsp {

/**
 * Plain old data (POD) structure used to record additional information and
 * precalculated values for a line in the current map.
 *
 * @ingroup bsp
 */
struct LineInfo
{
    /// @todo Refactor me away.
    enum Flag
    {
        Twosided   =        0x1, ///< Line is marked two-sided.
        ZeroLength =        0x2, ///< Zero length (line should be totally ignored).
        SelfRef    =        0x4  ///< Sector is the same on both sides.
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    /// The map line.
    Line *line;

    Flags flags;

    /// @todo Refactor me away.
    int validCount;

    /// If the line is used for a window effect, this is the sector on the back side.
    Sector *windowEffect;

    explicit LineInfo(Line *line_, coord_t distEpsilon = 0.0001)
        : line(line_), flags(0), validCount(0), windowEffect(0)
    {
        DENG2_ASSERT(line_);
        Vertex const &from = line->from();
        Vertex const &to   = line->to();

        // Check for zero-length line.
        if((de::abs(from.origin()[VX] - to.origin()[VX]) < distEpsilon) &&
           (de::abs(from.origin()[VY] - to.origin()[VY]) < distEpsilon))
            flags |= ZeroLength;

        if(line->hasFrontSections() && line->hasBackSections())
        {
            flags |= Twosided;

            if(line->isSelfReferencing())
                flags |= SelfRef;
        }
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(LineInfo::Flags)

} // namespace bsp
} // namespace de

#endif // LIBDENG_BSPBUILDER_LINEINFO