/** @file contactspreader.cpp World object => BSP leaf "contact" spreader.
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

#include <QBitArray>

#include <de/vector1.h>

#include "Face"
#include "HEdge"

#include "BspLeaf"
#include "Contact"
#include "Sector"
#include "Surface"

#include "render/r_main.h" // validCount
#include "render/rend_main.h" // Rend_mapSurfaceMaterialSpec
#include "WallEdge"
#include "MaterialSnapshot"

#include "world/contactspreader.h"

namespace de {

namespace internal {

/**
 * On which side of the half-edge does the specified @a point lie?
 *
 * @param hedge  Half-edge to test.
 * @param point  Point to test in the map coordinate space.
 *
 * @return @c <0 Point is to the left/back of the segment.
 *         @c =0 Point lies directly on the segment.
 *         @c >0 Point is to the right/front of the segment.
 */
static coord_t pointOnHEdgeSide(HEdge const &hedge, Vector2d const &point)
{
    Vector2d const direction = hedge.twin().origin() - hedge.origin();

    ddouble pointV1[2]      = { point.x, point.y };
    ddouble fromOriginV1[2] = { hedge.origin().x, hedge.origin().y };
    ddouble directionV1[2]  = { direction.x, direction.y };
    return V2d_PointOnLineSide(pointV1, fromOriginV1, directionV1);
}

struct ContactSpreader
{
    Blockmap const &_blockmap;
    QBitArray *_spreadBlocks;

    struct SpreadState
    {
        Contact *contact;
        AABoxd contactAABox;

        SpreadState() : contact(0) {}
    };
    SpreadState _spread;

    ContactSpreader(Blockmap const &blockmap, QBitArray *spreadBlocks = 0)
        : _blockmap(blockmap), _spreadBlocks(spreadBlocks)
    {}

    /**
     * Spread contacts in the blockmap to any touched neighbors.
     *
     * @param box   Map space region in which to perform spreading.
     */
    void spread(AABoxd const &box)
    {
        BlockmapCellBlock const cellBlock = _blockmap.toCellBlock(box);

        BlockmapCell cell;
        for(cell.y = cellBlock.min.y; cell.y < cellBlock.max.y; ++cell.y)
        for(cell.x = cellBlock.min.x; cell.x < cellBlock.max.x; ++cell.x)
        {
            if(_spreadBlocks)
            {
                // Should we skip this cell?
                int cellIndex = _blockmap.toCellIndex(cell.x, cell.y);
                if(_spreadBlocks->testBit(cellIndex))
                    continue;

                // Mark the cell as processed.
                _spreadBlocks->setBit(cellIndex);
            }

            _blockmap.iterate(cell, spreadContactWorker, this);
        }
    }

private:
    /**
     * Link the contact in all BspLeafs which touch the linked object (tests are
     * done with bounding boxes and the BSP leaf spread test).
     *
     * @param contact  Contact to be spread.
     */
    void spreadContact(Contact &contact)
    {
        BspLeaf &bspLeaf = contact.objectBspLeafAtOrigin();
        DENG2_ASSERT(bspLeaf.hasCluster()); // Sanity check.

        R_ContactList(bspLeaf, contact.type()).link(&contact);

        // Spread to neighboring BSP leafs.
        bspLeaf.setValidCount(++validCount);

        _spread.contact      = &contact;
        _spread.contactAABox = contact.objectAABox();

        spreadInBspLeaf(bspLeaf);
    }

    void maybeSpreadOverEdge(HEdge *hedge)
    {
        DENG2_ASSERT(_spread.contact != 0);

        if(!hedge) return;

        BspLeaf &leaf = hedge->face().mapElementAs<BspLeaf>();
        SectorCluster &cluster = leaf.cluster();

        // There must be a back BSP leaf to spread to.
        if(!hedge->hasTwin()) return;
        if(!hedge->twin().hasFace()) return;

        BspLeaf &backLeaf = hedge->twin().face().mapElementAs<BspLeaf>();
        if(!backLeaf.hasCluster()) return;

        SectorCluster &backCluster = backLeaf.cluster();

        // Which way does the spread go?
        if(!(leaf.validCount() == validCount && backLeaf.validCount() != validCount))
        {
            return; // Not eligible for spreading.
        }

        // Is the leaf on the back side outside the origin's AABB?
        AABoxd const &aaBox = backLeaf.poly().aaBox();
        if(aaBox.maxX <= _spread.contactAABox.minX || aaBox.minX >= _spread.contactAABox.maxX ||
           aaBox.maxY <= _spread.contactAABox.minY || aaBox.minY >= _spread.contactAABox.maxY)
            return;

        // Too far from the edge?
        coord_t const length   = (hedge->twin().origin() - hedge->origin()).length();
        coord_t const distance = pointOnHEdgeSide(*hedge, _spread.contact->objectOrigin()) / length;
        if(abs(distance) >= _spread.contact->objectRadius())
            return;

        // Do not spread if the sector on the back side is closed with no height.
        if(!backCluster.hasWorldVolume())
            return;

        if(backCluster.visCeiling().heightSmoothed() <= cluster.visFloor().heightSmoothed() ||
           backCluster.visFloor().heightSmoothed() >= cluster.visCeiling().heightSmoothed())
            return;

        // Are there line side surfaces which should prevent spreading?
        if(hedge->hasMapElement())
        {
            LineSideSegment const &seg = hedge->mapElementAs<LineSideSegment>();

            // On which side of the line are we? (distance is from segment to origin).
            LineSide const &facingLineSide = seg.line().side(seg.lineSide().sideId() ^ (distance < 0));

            // One-way window?
            if(!facingLineSide.back().hasSections())
                return;

            SectorCluster const &fromCluster = facingLineSide.isFront()? cluster : backCluster;
            SectorCluster const &toCluster   = facingLineSide.isFront()? backCluster : cluster;

            // Might a material cover the opening?
            if(facingLineSide.hasSections() && facingLineSide.middle().hasMaterial())
            {
                // Stretched middles always cover the opening.
                if(facingLineSide.isFlagged(SDF_MIDDLE_STRETCH))
                    return;

                // Determine the opening between the visual sector planes at this edge.
                coord_t openBottom;
                if(toCluster.visFloor().heightSmoothed() > fromCluster.visFloor().heightSmoothed())
                {
                    openBottom = toCluster.visFloor().heightSmoothed();
                }
                else
                {
                    openBottom = fromCluster.visFloor().heightSmoothed();
                }

                coord_t openTop;
                if(toCluster.visCeiling().heightSmoothed() < fromCluster.visCeiling().heightSmoothed())
                {
                    openTop = toCluster.visCeiling().heightSmoothed();
                }
                else
                {
                    openTop = fromCluster.visCeiling().heightSmoothed();
                }

                // Ensure we have up to date info about the material.
                MaterialSnapshot const &ms = facingLineSide.middle().material().prepare(Rend_MapSurfaceMaterialSpec());
                if(ms.height() >= openTop - openBottom)
                {
                    // Possibly; check the placement.
                    WallEdge edge(WallSpec::fromMapSide(facingLineSide, LineSide::Middle),
                                     *facingLineSide.leftHEdge(), Line::From);

                    if(edge.isValid() && edge.top().z() > edge.bottom().z() &&
                       edge.top().z() >= openTop && edge.bottom().z() <= openBottom)
                        return;
                }
            }
        }

        // During the next step this contact will spread from the back leaf.
        backLeaf.setValidCount(validCount);

        R_ContactList(backLeaf, _spread.contact->type()).link(_spread.contact);

        spreadInBspLeaf(backLeaf);
    }

    /**
     * Attempt to spread the obj from the given contact from the source
     * BspLeaf and into the (relative) back BspLeaf.
     *
     * @param bspLeaf  BspLeaf to attempt to spread over to.
     *
     * @return  Always @c true. (This function is also used as an iterator.)
     */
    void spreadInBspLeaf(BspLeaf &bspLeaf)
    {
        if(bspLeaf.hasCluster())
        {
            HEdge *base = bspLeaf.poly().hedge();
            HEdge *hedge = base;
            do
            {
                maybeSpreadOverEdge(hedge);

            } while((hedge = &hedge->next()) != base);
        }
    }

    static int spreadContactWorker(void *contact, void *context)
    {
        ContactSpreader *inst = static_cast<ContactSpreader *>(context);
        inst->spreadContact(*static_cast<Contact *>(contact));
        return false; // Continue iteration.
    }
};

} // internal

void spreadContacts(Blockmap const &blockmap, AABoxd const &region,
    QBitArray *spreadBlocks)
{
    internal::ContactSpreader(blockmap, spreadBlocks).spread(region);
}

} // namespace de
