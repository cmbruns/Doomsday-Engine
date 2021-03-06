/** @file plane.h World map plane.
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
 * General Public License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA</small>
 */

#include <de/Log>

#include "world/map.h"
#include "world/world.h" /// ddMapSetup
#include "Surface"
#include "Sector"

#include "render/r_main.h" // frameTimePos

#include "world/plane.h"

using namespace de;

DENG2_PIMPL(Plane)
{
    SoundEmitter soundEmitter;
    int indexInSector;           ///< Index in the owning sector.
    coord_t height;              ///< Current @em sharp height.
    coord_t targetHeight;        ///< Target @em sharp height.
    coord_t speed;               ///< Movement speed (map space units per tic).
    Surface surface;

#ifdef __CLIENT__
    coord_t oldHeight[2];        ///< @em sharp height change tracking buffer (for smoothing).
    coord_t heightSmoothed;      ///< @ref height (smoothed).
    coord_t heightSmoothedDelta; ///< Delta between the current @em sharp height and the visual height.
#endif

    Instance(Public *i, coord_t height)
        : Base(i),
          indexInSector(-1),
          height(height),
          targetHeight(height),
          speed(0),
          surface(dynamic_cast<MapElement &>(*i))
#ifdef __CLIENT__
         ,heightSmoothed(height),
          heightSmoothedDelta(0)
#endif
    {
#ifdef __CLIENT__
        oldHeight[0] = oldHeight[1] = height;
#endif
        zap(soundEmitter);
    }

    ~Instance()
    {
        DENG2_FOR_PUBLIC_AUDIENCE(Deletion, i) i->planeBeingDeleted(self);

#ifdef __CLIENT__
        // Stop movement tracking of this plane.
        self.map().trackedPlanes().remove(&self);
#endif
    }

    void notifyHeightChanged(coord_t oldHeight)
    {
        DENG2_FOR_PUBLIC_AUDIENCE(HeightChange, i)
        {
            i->planeHeightChanged(self, oldHeight);
        }
    }

#ifdef __CLIENT__
    void notifySmoothedHeightChanged(coord_t oldHeight)
    {
        DENG2_FOR_PUBLIC_AUDIENCE(HeightSmoothedChange, i)
        {
            i->planeHeightSmoothedChanged(self, oldHeight);
        }
    }
#endif

    void applySharpHeightChange(coord_t newHeight)
    {
        // No change?
        if(de::fequal(newHeight, height))
            return;

        coord_t oldHeight = height;
        height = newHeight;

        if(!ddMapSetup)
        {
            // Update the sound emitter origin for the plane.
            self.updateSoundEmitterOrigin();

#ifdef __CLIENT__
            // We need the decorations updated.
            /// @todo optimize: Translation on the world up axis would be a
            /// trivial operation to perform, which, would not require plotting
            /// decorations again. This frequent case should be designed for.
            surface.markAsNeedingDecorationUpdate();
#endif
        }

        // Notify interested parties of the change.
        notifyHeightChanged(oldHeight);

#ifdef __CLIENT__
        if(!ddMapSetup)
        {
            // Add ourself to tracked plane list (for movement interpolation).
            self.map().trackedPlanes().insert(&self);
        }
#endif
    }
};

Plane::Plane(Sector &sector, Vector3f const &normal, coord_t height)
    : MapElement(DMU_PLANE, &sector), d(new Instance(this, height))
{
    setNormal(normal);
}

Sector &Plane::sector()
{
    return parent().as<Sector>();
}

Sector const &Plane::sector() const
{
    return parent().as<Sector>();
}

int Plane::indexInSector() const
{
    return d->indexInSector;
}

void Plane::setIndexInSector(int newIndex)
{
    d->indexInSector = newIndex;
}

bool Plane::isSectorFloor() const
{
    return this == &sector().floor();
}

bool Plane::isSectorCeiling() const
{
    return this == &sector().ceiling();
}

Surface &Plane::surface()
{
    return d->surface;
}

Surface const &Plane::surface() const
{
    return d->surface;
}

void Plane::setNormal(Vector3f const &newNormal)
{
    d->surface.setNormal(newNormal); // will normalize
}

SoundEmitter &Plane::soundEmitter()
{
    return d->soundEmitter;
}

SoundEmitter const &Plane::soundEmitter() const
{
    return d->soundEmitter;
}

void Plane::updateSoundEmitterOrigin()
{
    LOG_AS("Plane::updateSoundEmitterOrigin");

    d->soundEmitter.origin[VX] = sector().soundEmitter().origin[VX];
    d->soundEmitter.origin[VY] = sector().soundEmitter().origin[VY];
    d->soundEmitter.origin[VZ] = d->height;
}

coord_t Plane::height() const
{
    return d->height;
}

coord_t Plane::targetHeight() const
{
    return d->targetHeight;
}

coord_t Plane::speed() const
{
    return d->speed;
}

#ifdef __CLIENT__

coord_t Plane::heightSmoothed() const
{
    return d->heightSmoothed;
}

coord_t Plane::heightSmoothedDelta() const
{
    return d->heightSmoothedDelta;
}

void Plane::lerpSmoothedHeight()
{
    // Interpolate.
    d->heightSmoothedDelta = d->oldHeight[0] * (1 - frameTimePos)
                           + d->height * frameTimePos - d->height;

    coord_t newHeightSmoothed = d->height + d->heightSmoothedDelta;
    if(!de::fequal(d->heightSmoothed, newHeightSmoothed))
    {
        coord_t oldHeightSmoothed = d->heightSmoothed;
        d->heightSmoothed = newHeightSmoothed;
        d->notifySmoothedHeightChanged(oldHeightSmoothed);
    }
}

void Plane::resetSmoothedHeight()
{
    // Reset interpolation.
    d->heightSmoothedDelta = 0;

    coord_t newHeightSmoothed = d->oldHeight[0] = d->oldHeight[1] = d->height;
    if(!de::fequal(d->heightSmoothed, newHeightSmoothed))
    {
        coord_t oldHeightSmoothed = d->heightSmoothed;
        d->heightSmoothed = newHeightSmoothed;
        d->notifySmoothedHeightChanged(oldHeightSmoothed);
    }
}

void Plane::updateHeightTracking()
{
    d->oldHeight[0] = d->oldHeight[1];
    d->oldHeight[1] = d->height;

    if(!de::fequal(d->oldHeight[0], d->oldHeight[1]))
    {
        if(de::abs(d->oldHeight[0] - d->oldHeight[1]) >= MAX_SMOOTH_MOVE)
        {
            // Too fast: make an instantaneous jump.
            d->oldHeight[0] = d->oldHeight[1];
        }
    }
}

#endif // __CLIENT__

int Plane::property(DmuArgs &args) const
{
    switch(args.prop)
    {
    case DMU_EMITTER:
        args.setValue(DMT_PLANE_EMITTER, &d->soundEmitter, 0);
        break;
    case DMU_SECTOR: {
        Sector const *secPtr = &sector();
        args.setValue(DMT_PLANE_SECTOR, &secPtr, 0);
        break; }
    case DMU_HEIGHT:
        args.setValue(DMT_PLANE_HEIGHT, &d->height, 0);
        break;
    case DMU_TARGET_HEIGHT:
        args.setValue(DMT_PLANE_TARGET, &d->targetHeight, 0);
        break;
    case DMU_SPEED:
        args.setValue(DMT_PLANE_SPEED, &d->speed, 0);
        break;
    default:
        return MapElement::property(args);
    }

    return false; // Continue iteration.
}

int Plane::setProperty(DmuArgs const &args)
{
    switch(args.prop)
    {
    case DMU_HEIGHT: {
        coord_t newHeight = d->height;
        args.value(DMT_PLANE_HEIGHT, &newHeight, 0);
        d->applySharpHeightChange(newHeight);
        break; }
    case DMU_TARGET_HEIGHT:
        args.value(DMT_PLANE_TARGET, &d->targetHeight, 0);
        break;
    case DMU_SPEED:
        args.value(DMT_PLANE_SPEED, &d->speed, 0);
        break;
    default:
        return MapElement::setProperty(args);
    }

    return false; // Continue iteration.
}
