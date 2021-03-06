/** @file render/skyfixedge.cpp Sky Fix Edge Geometry.
 *
 * @authors Copyright © 2011-2013 Daniel Swanson <danij@dengine.net>
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

#include "dd_main.h"

#include "Face"

#include "BspLeaf"
#include "Plane"
#include "Sector"
#include "Surface"

#include "world/map.h"
#include "world/maputil.h" // R_SideBackClosed, remove me
#include "world/p_players.h"

#include "render/rend_main.h"

#include "render/skyfixedge.h"

namespace de {

static coord_t skyFixFloorZ(Plane const *frontFloor, Plane const *backFloor)
{
    DENG_UNUSED(backFloor);
    if(devRendSkyMode || P_IsInVoid(viewPlayer))
        return frontFloor->heightSmoothed();
    return frontFloor->map().skyFixFloor();
}

static coord_t skyFixCeilZ(Plane const *frontCeil, Plane const *backCeil)
{
    DENG_UNUSED(backCeil);
    if(devRendSkyMode || P_IsInVoid(viewPlayer))
        return frontCeil->heightSmoothed();
    return frontCeil->map().skyFixCeiling();
}

DENG2_PIMPL_NOREF(SkyFixEdge::Event)
{
    SkyFixEdge &owner;
    double distance;

    Instance(SkyFixEdge &owner, double distance)
        : owner(owner), distance(distance)
    {}
};

SkyFixEdge::Event::Event(SkyFixEdge &owner, double distance)
    : WorldEdge::Event(),
      d(new Instance(owner, distance))
{}

bool SkyFixEdge::Event::operator < (Event const &other) const
{
    return d->distance < other.distance();
}

double SkyFixEdge::Event::distance() const
{
    return d->distance;
}

Vector3d SkyFixEdge::Event::origin() const
{
    return d->owner.pOrigin() + d->owner.pDirection() * distance();
}

DENG2_PIMPL(SkyFixEdge)
{
    HEdge *hedge;
    FixType fixType;
    int edge;

    Vector3d pOrigin;
    Vector3d pDirection;

    coord_t lo, hi;

    Event bottom;
    Event top;
    bool isValid;

    Vector2f materialOrigin;

    Instance(Public *i, HEdge &hedge, FixType fixType, int edge,
             Vector2f const &materialOrigin)
        : Base(i),
          hedge(&hedge),
          fixType(fixType),
          edge(edge),
          bottom(*i, 0),
          top(*i, 1),
          isValid(false),
          materialOrigin(materialOrigin)
    {}

    /**
     * Determines whether a sky fix is actually necessary.
     */
    bool wallSectionNeedsSkyFix() const
    {
        DENG_ASSERT(hedge->hasFace());

        bool const lower = fixType == SkyFixEdge::Lower;

        // Only edges with line segments need fixes.
        if(!hedge->hasMapElement()) return false;

        SectorCluster const *cluster     = hedge->face().mapElementAs<BspLeaf>().clusterPtr();
        SectorCluster const *backCluster = hedge->twin().hasFace()? hedge->twin().face().mapElementAs<BspLeaf>() .clusterPtr() : 0;

        if(backCluster && &backCluster->sector() == &cluster->sector())
            return false;

        // Select the relative planes for the fix type.
        int relPlane = lower? Sector::Floor : Sector::Ceiling;
        Plane const *front   = &cluster->visPlane(relPlane);
        Plane const *back    = backCluster? &backCluster->visPlane(relPlane) : 0;

        if(!front->surface().hasSkyMaskedMaterial())
            return false;

        LineSide const &lineSide = hedge->mapElementAs<LineSideSegment>().lineSide();
        bool const hasClosedBack = R_SideBackClosed(lineSide);

        if(!devRendSkyMode)
        {
            if(!P_IsInVoid(viewPlayer) &&
               !(hasClosedBack || !(back && back->surface().hasSkyMaskedMaterial())))
                return false;
        }
        else
        {
            int relSection = lower? LineSide::Bottom : LineSide::Top;

            if(lineSide.surface(relSection).hasMaterial() ||
               !(hasClosedBack || (back && back->surface().hasSkyMaskedMaterial())))
                return false;
        }

        // Figure out the relative plane heights.
        coord_t fz = front->heightSmoothed();
        if(relPlane == Sector::Ceiling)
            fz = -fz;

        coord_t bz = 0;
        if(back)
        {
            bz = back->heightSmoothed();
            if(relPlane == Sector::Ceiling)
                bz = -bz;
        }

        coord_t planeZ = (back && back->surface().hasSkyMaskedMaterial() &&
                          fz < bz? bz : fz);

        coord_t skyZ = lower? skyFixFloorZ(front, back)
                            : -skyFixCeilZ(front, back);

        return (planeZ > skyZ);
    }

    void prepare()
    {
        if(!wallSectionNeedsSkyFix())
        {
            isValid = false;
            return;
        }

        SectorCluster const *cluster     = hedge->face().mapElementAs<BspLeaf>().clusterPtr();
        SectorCluster const *backCluster =
            hedge->twin().hasFace()? hedge->twin().face().mapElementAs<BspLeaf>().clusterPtr() : 0;

        Plane const *ffloor = &cluster->visFloor();
        Plane const *fceil  = &cluster->visCeiling();
        Plane const *bceil  = backCluster? &backCluster->visCeiling() : 0;
        Plane const *bfloor = backCluster? &backCluster->visFloor()   : 0;

        if(fixType == Upper)
        {
            hi = skyFixCeilZ(fceil, bceil);
            lo = de::max((backCluster && bceil->surface().hasSkyMaskedMaterial())? bceil->heightSmoothed()
                                                                                 : fceil->heightSmoothed(),
                         ffloor->heightSmoothed());
        }
        else
        {
            hi = de::min((backCluster && bfloor->surface().hasSkyMaskedMaterial())? bfloor->heightSmoothed()
                                                                                  : ffloor->heightSmoothed(),
                         fceil->heightSmoothed());
            lo = skyFixFloorZ(ffloor, bfloor);
        }

        isValid = hi > lo;
        if(!isValid)
            return;

        pOrigin = Vector3d(self.origin(), lo);
        pDirection = Vector3d(0, 0, hi - lo);
    }
};

SkyFixEdge::SkyFixEdge(HEdge &hedge, FixType fixType, int edge, float materialOffsetS)
    : WorldEdge((edge? hedge.twin() : hedge).origin()),
      d(new Instance(this, hedge, fixType, edge, Vector2f(materialOffsetS, 0)))
{
    /// @todo Defer until necessary.
    d->prepare();
}

Vector3d const &SkyFixEdge::pOrigin() const
{
    return d->pOrigin;
}

Vector3d const &SkyFixEdge::pDirection() const
{
    return d->pDirection;
}

Vector2f SkyFixEdge::materialOrigin() const
{
    return d->materialOrigin;
}

bool SkyFixEdge::isValid() const
{
    return d->isValid;
}

SkyFixEdge::Event const &SkyFixEdge::first() const
{
    return d->bottom;
}

SkyFixEdge::Event const &SkyFixEdge::last() const
{
    return d->top;
}

SkyFixEdge::Event const &SkyFixEdge::at(EventIndex index) const
{
    if(index >= 0 && index < 2)
    {
        return index == 0? d->bottom : d->top;
    }
    /// @throw InvalidIndexError The specified event index is not valid.
    throw Error("SkyFixEdge::at", QString("Index '%1' does not map to a known event (count: 2)").arg(index));
}

} // namespace de
