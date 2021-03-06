/** @file bspleaf.cpp World map BSP leaf.
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

#include <QMap>
#include <QtAlgorithms>

#include <de/Log>

#include "Face"

#include "Polyobj"
#include "Sector"
#include "Surface"
#ifdef __CLIENT__
#  include "world/map.h"
#endif

#ifdef __CLIENT__
#  include "BiasDigest"
#  include "BiasIllum"
#  include "BiasSource"
#  include "BiasTracker"
#endif

#include "world/bspleaf.h"

using namespace de;

/// Compute the area of a triangle defined by three 2D point vectors.
ddouble triangleArea(Vector2d const &v1, Vector2d const &v2, Vector2d const &v3)
{
    Vector2d a = v2 - v1;
    Vector2d b = v3 - v1;
    return (a.x * b.y - b.x * a.y) / 2;
}

#ifdef __CLIENT__
struct GeometryGroup
{
    typedef QList<BiasIllum> BiasIllums;

    uint biasLastUpdateFrame;
    BiasIllums biasIllums;
    BiasTracker biasTracker;
};

/// Geometry group identifier => group data.
typedef QMap<int, GeometryGroup> GeometryGroups;
#endif

DENG2_PIMPL(BspLeaf)
{
    SectorCluster *cluster;         ///< Attributed cluster (if any, not owned).

    Face *poly;                     ///< Convex polygon geometry (if any, not owned).
    Meshes extraMeshes;             ///< Additional meshes (owned).
    Polyobjs polyobjs;              ///< Linked polyobjs (if any, not owned).

    /// Offset to align the top left of materials in the built geometry to the
    /// map coordinate space grid.
    Vector2d worldGridOffset;

#ifdef __CLIENT__
    HEdge *fanBase;                 ///< Trifan base Half-edge (otherwise the center point is used).

    bool needUpdateFanBase;         ///< @c true= need to rechoose a fan base half-edge.
    int addSpriteCount;             ///< Frame number of last R_AddSprites.

    GeometryGroups geomGroups;
    Lumobjs lumobjs;                ///< Linked lumobjs (not owned).
    ShadowLines shadowLines;        ///< Linked map lines for fake radio shadowing.
    AudioEnvironmentFactors reverb; ///< Cached characteristics.
#endif // __CLIENT__

    /// Used by legacy algorithms to prevent repeated processing.
    int validCount;

    Instance(Public *i)
        : Base(i),
          cluster(0),
          poly(0),
#ifdef __CLIENT__
          fanBase(0),
          needUpdateFanBase(true),
          addSpriteCount(0),
#endif
          validCount(0)
    {
#ifdef __CLIENT__
        zap(reverb);
#endif
    }

    ~Instance()
    {
        qDeleteAll(extraMeshes);
    }

#ifdef __CLIENT__

    /**
     * Determine the half-edge whose vertex is suitable for use as the center point
     * of a trifan primitive.
     *
     * Note that we do not want any overlapping or zero-area (degenerate) triangles.
     *
     * @par Algorithm
     * <pre>For each vertex
     *    For each triangle
     *        if area is not greater than minimum bound, move to next vertex
     *    Vertex is suitable
     * </pre>
     *
     * If a vertex exists which results in no zero-area triangles it is suitable for
     * use as the center of our trifan. If a suitable vertex is not found then the
     * center of BSP leaf should be selected instead (it will always be valid as
     * BSP leafs are convex).
     */
    void chooseFanBase()
    {
#define MIN_TRIANGLE_EPSILON  (0.1) ///< Area

        HEdge *firstNode = poly->hedge();

        fanBase = firstNode;

        if(poly->hedgeCount() > 3)
        {
            // Splines with higher vertex counts demand checking.
            Vertex const *base, *a, *b;

            // Search for a good base.
            do
            {
                HEdge *other = firstNode;

                base = &fanBase->vertex();
                do
                {
                    // Test this triangle?
                    if(!(fanBase != firstNode && (other == fanBase || other == &fanBase->prev())))
                    {
                        a = &other->vertex();
                        b = &other->next().vertex();

                        if(de::abs(triangleArea(base->origin(), a->origin(), b->origin())) <= MIN_TRIANGLE_EPSILON)
                        {
                            // No good. We'll move on to the next vertex.
                            base = 0;
                        }
                    }

                    // On to the next triangle.
                } while(base && (other = &other->next()) != firstNode);

                if(!base)
                {
                    // No good. Select the next vertex and start over.
                    fanBase = &fanBase->next();
                }
            } while(!base && fanBase != firstNode);

            // Did we find something suitable?
            if(!base) // No.
            {
                fanBase = 0;
            }
        }
        //else Implicitly suitable (or completely degenerate...).

        needUpdateFanBase = false;

#undef MIN_TRIANGLE_EPSILON
    }

    /**
     * Retrieve geometry data by it's associated unique @a group identifier.
     *
     * @param group     Geometry group identifier.
     * @param canAlloc  @c true= to allocate if no data exists. Note that the
     *                  number of vertices in the fan geometry must be known
     *                  at this time.
     */
    GeometryGroup *geometryGroup(int group, bool canAlloc = true)
    {
        DENG_ASSERT(cluster != 0); // sanity check
        DENG_ASSERT(group >= 0 && group < cluster->sector().planeCount()); // sanity check

        GeometryGroups::iterator foundAt = geomGroups.find(group);
        if(foundAt != geomGroups.end())
        {
            return &*foundAt;
        }

        if(!canAlloc) return 0;

        // Number of bias illumination points for this geometry. Presently we
        // define a 1:1 mapping to fan geometry vertices.
        int numBiasIllums = self.numFanVertices();

        GeometryGroup &newGeomGroup = *geomGroups.insert(group, GeometryGroup());
        newGeomGroup.biasIllums.reserve(numBiasIllums);
        for(int i = 0; i < numBiasIllums; ++i)
        {
            newGeomGroup.biasIllums.append(BiasIllum(&newGeomGroup.biasTracker));
        }

        return &newGeomGroup;
    }

    /**
     * @todo This could be enhanced so that only the lights on the right
     * side of the surface are taken into consideration.
     */
    void updateBiasContributors(GeometryGroup &geomGroup, int planeIndex)
    {
        // If the data is already up to date, nothing needs to be done.
        uint lastChangeFrame = self.map().biasLastChangeOnFrame();
        if(geomGroup.biasLastUpdateFrame == lastChangeFrame)
            return;

        geomGroup.biasLastUpdateFrame = lastChangeFrame;

        geomGroup.biasTracker.clearContributors();

        Plane const &plane     = cluster->visPlane(planeIndex);
        Surface const &surface = plane.surface();

        Vector3d surfacePoint(poly->center(), plane.heightSmoothed());

        foreach(BiasSource *source, self.map().biasSources())
        {
            // If the source is too weak we will ignore it completely.
            if(source->intensity() <= 0)
                continue;

            Vector3d sourceToSurface = (source->origin() - surfacePoint).normalize();
            coord_t distance = 0;

            // Calculate minimum 2D distance to the BSP leaf.
            /// @todo This is probably too accurate an estimate.
            HEdge *baseNode = poly->hedge();
            HEdge *node = baseNode;
            do
            {
                coord_t len = (Vector2d(source->origin()) - node->origin()).length();
                if(node == baseNode || len < distance)
                    distance = len;
            } while((node = &node->next()) != baseNode);

            if(sourceToSurface.dot(surface.normal()) < 0)
                continue;

            geomGroup.biasTracker.addContributor(source, source->evaluateIntensity() / de::max(distance, 1.0));
        }
    }

#endif // __CLIENT__
};

BspLeaf::BspLeaf(Sector *sector)
    : MapElement(DMU_BSPLEAF, sector), d(new Instance(this))
{}

bool BspLeaf::hasPoly() const
{
    return d->poly != 0;
}

Face const &BspLeaf::poly() const
{
    if(d->poly)
    {
        return *d->poly;
    }
    /// @throw MissingPolyError Attempted with no polygon assigned.
    throw MissingPolyError("BspLeaf::poly", "No polygon is assigned");
}

void BspLeaf::setPoly(Face *newPoly)
{
    if(d->poly == newPoly) return;

    if(newPoly && !newPoly->isConvex())
    {
        /// @throw InvalidPolyError Attempted to attribute a non-convex polygon.
        throw InvalidPolyError("BspLeaf::setPoly", "Non-convex polygons cannot be assigned");
    }

    d->poly = newPoly;

    if(newPoly)
    {
        // Attribute the new face geometry to "this" BSP leaf.
        newPoly->setMapElement(this);

        // Update the world grid offset.
        d->worldGridOffset = Vector2d(fmod(newPoly->aaBox().minX, 64),
                                      fmod(newPoly->aaBox().maxY, 64));
    }
    else
    {
        d->worldGridOffset = Vector2d(0, 0);
    }
}

void BspLeaf::assignExtraMesh(Mesh &newMesh)
{
    int const sizeBefore = d->extraMeshes.size();

    d->extraMeshes.insert(&newMesh);

    if(d->extraMeshes.size() != sizeBefore)
    {
        LOG_AS("BspLeaf");
        LOG_DEBUG("Assigned extra mesh to leaf [%p].") << de::dintptr(this);

        // Attribute all faces to "this" BSP leaf.
        foreach(Face *face, newMesh.faces())
        {
            face->setMapElement(this);
        }
    }
}

BspLeaf::Meshes const &BspLeaf::extraMeshes() const
{
    return d->extraMeshes;
}

Vector2d const &BspLeaf::worldGridOffset() const
{
    return d->worldGridOffset;
}

bool BspLeaf::hasCluster() const
{
    return d->cluster != 0;
}

SectorCluster &BspLeaf::cluster() const
{
    if(d->cluster)
    {
        return *d->cluster;
    }
    /// @throw MissingClusterError Attempted with no sector cluster attributed.
    throw MissingClusterError("BspLeaf::cluster", "No sector cluster is attributed");
}

void BspLeaf::setCluster(SectorCluster *newCluster)
{
    d->cluster = newCluster;
}

void BspLeaf::link(Polyobj const &polyobj)
{
    d->polyobjs.insert(const_cast<Polyobj *>(&polyobj));
}

bool BspLeaf::unlink(polyobj_s const &polyobj)
{
    int sizeBefore = d->polyobjs.size();
    d->polyobjs.remove(const_cast<Polyobj *>(&polyobj));
    return d->polyobjs.size() != sizeBefore;
}

BspLeaf::Polyobjs const &BspLeaf::polyobjs() const
{
    return d->polyobjs;
}

int BspLeaf::validCount() const
{
    return d->validCount;
}

void BspLeaf::setValidCount(int newValidCount)
{
    d->validCount = newValidCount;
}

bool BspLeaf::polyContains(Vector2d const &point) const
{
    if(!hasPoly()) return false; // Obviously not.

    HEdge const *hedge = poly().hedge();
    do
    {
        Vertex const &va = hedge->vertex();
        Vertex const &vb = hedge->next().vertex();

        if(((va.origin().y - point.y) * (vb.origin().x - va.origin().x) -
            (va.origin().x - point.x) * (vb.origin().y - va.origin().y)) < 0)
        {
            // Outside the BSP leaf's edges.
            return false;
        }

    } while((hedge = &hedge->next()) != poly().hedge());

    return true;
}

#ifdef __CLIENT__

HEdge *BspLeaf::fanBase() const
{
    if(d->needUpdateFanBase)
    {
        d->chooseFanBase();
    }
    return d->fanBase;
}

int BspLeaf::numFanVertices() const
{
    // Are we to use one of the half-edge vertexes as the fan base?
    if(!d->poly) return 0;
    return d->poly->hedgeCount() + (fanBase()? 0 : 2);
}

static void updateBiasForWallSectionsAfterGeometryMove(HEdge *hedge)
{
    if(!hedge || !hedge->hasMapElement())
        return;

    LineSideSegment &seg = hedge->mapElementAs<LineSideSegment>();
    seg.updateBiasAfterGeometryMove(LineSide::Middle);
    seg.updateBiasAfterGeometryMove(LineSide::Bottom);
    seg.updateBiasAfterGeometryMove(LineSide::Top);
}

void BspLeaf::updateBiasAfterGeometryMove(int group)
{
    if(!hasPoly()) return;

    if(GeometryGroup *geomGroup = d->geometryGroup(group, false /*don't allocate*/))
    {
        geomGroup->biasTracker.updateAllContributors();
    }

    HEdge *base = poly().hedge();
    HEdge *hedge = base;
    do
    {
        updateBiasForWallSectionsAfterGeometryMove(hedge);
    } while((hedge = &hedge->next()) != base);

    foreach(Mesh *mesh, extraMeshes())
    foreach(HEdge *hedge, mesh->hedges())
    {
        updateBiasForWallSectionsAfterGeometryMove(hedge);
    }
}

static void applyBiasDigestToWallSections(HEdge *hedge, BiasDigest &changes)
{
    if(!hedge || !hedge->hasMapElement())
        return;
    hedge->mapElementAs<LineSideSegment>().applyBiasDigest(changes);
}

void BspLeaf::applyBiasDigest(BiasDigest &changes)
{
    if(!hasPoly()) return;

    for(GeometryGroups::iterator it = d->geomGroups.begin();
        it != d->geomGroups.end(); ++it)
    {
        it.value().biasTracker.applyChanges(changes);
    }

    HEdge *base = poly().hedge();
    HEdge *hedge = base;
    do
    {
        applyBiasDigestToWallSections(hedge, changes);
    } while((hedge = &hedge->next()) != base);

    foreach(Mesh *mesh, extraMeshes())
    foreach(HEdge *hedge, mesh->hedges())
    {
        applyBiasDigestToWallSections(hedge, changes);
    }

    foreach(Polyobj *polyobj, d->polyobjs)
    foreach(HEdge *hedge, polyobj->mesh().hedges())
    {
        applyBiasDigestToWallSections(hedge, changes);
    }
}

void BspLeaf::lightBiasPoly(int group, Vector3f const *posCoords, Vector4f *colorCoords)
{
    DENG_ASSERT(posCoords != 0 && colorCoords != 0);

    int const planeIndex     = group;
    GeometryGroup *geomGroup = d->geometryGroup(planeIndex);

    // Should we update?
    if(devUpdateBiasContributors)
    {
        d->updateBiasContributors(*geomGroup, planeIndex);
    }

    Surface const &surface = cluster().visPlane(planeIndex).surface();
    uint const biasTime = map().biasCurrentTime();

    Vector3f const *posIt = posCoords;
    Vector4f *colorIt     = colorCoords;
    for(int i = 0; i < geomGroup->biasIllums.count(); ++i, posIt++, colorIt++)
    {
        *colorIt += geomGroup->biasIllums[i].evaluate(*posIt, surface.normal(), biasTime);
    }

    // Any changes from contributors will have now been applied.
    geomGroup->biasTracker.markIllumUpdateCompleted();
}

static void accumReverbForWallSections(HEdge const *hedge,
    float envSpaceAccum[NUM_AUDIO_ENVIRONMENTS], float &total)
{
    // Edges with no map line segment implicitly have no surfaces.
    if(!hedge || !hedge->hasMapElement())
        return;

    LineSideSegment const &seg = hedge->mapElementAs<LineSideSegment>();
    if(!seg.lineSide().hasSections() || !seg.lineSide().middle().hasMaterial())
        return;

    Material &material = seg.lineSide().middle().material();
    AudioEnvironmentId env = material.audioEnvironment();
    if(!(env >= 0 && env < NUM_AUDIO_ENVIRONMENTS))
        env = AE_WOOD; // Assume it's wood if unknown.

    total += seg.length();

    envSpaceAccum[env] += seg.length();
}

bool BspLeaf::updateReverb()
{
    if(!hasCluster())
    {
        d->reverb[SRD_SPACE] = d->reverb[SRD_VOLUME] =
            d->reverb[SRD_DECAY] = d->reverb[SRD_DAMPING] = 0;
        return false;
    }

    float envSpaceAccum[NUM_AUDIO_ENVIRONMENTS];
    std::memset(&envSpaceAccum, 0, sizeof(envSpaceAccum));

    // Space is the rough volume of the BSP leaf (bounding box).
    AABoxd const &aaBox = d->poly->aaBox();
    d->reverb[SRD_SPACE] = int(cluster().ceiling().height() - cluster().floor().height())
                         * (aaBox.maxX - aaBox.minX) * (aaBox.maxY - aaBox.minY);

    // The other reverb properties can be found out by taking a look at the
    // materials of all surfaces in the BSP leaf.
    float total  = 0;
    HEdge *base  = d->poly->hedge();
    HEdge *hedge = base;
    do
    {
        accumReverbForWallSections(hedge, envSpaceAccum, total);
    } while((hedge = &hedge->next()) != base);

    foreach(Mesh *mesh, d->extraMeshes)
    foreach(HEdge *hedge, mesh->hedges())
    {
        accumReverbForWallSections(hedge, envSpaceAccum, total);
    }

    if(!total)
    {
        // Huh?
        d->reverb[SRD_VOLUME] = d->reverb[SRD_DECAY] = d->reverb[SRD_DAMPING] = 0;
        return false;
    }

    // Average the results.
    for(int i = AE_FIRST; i < NUM_AUDIO_ENVIRONMENTS; ++i)
    {
        envSpaceAccum[i] /= total;
    }

    // Accumulate and clamp the final characteristics
    int accum[NUM_REVERB_DATA]; zap(accum);
    for(int i = AE_FIRST; i < NUM_AUDIO_ENVIRONMENTS; ++i)
    {
        AudioEnvironment const &envInfo = S_AudioEnvironment(AudioEnvironmentId(i));
        // Volume.
        accum[SRD_VOLUME]  += envSpaceAccum[i] * envInfo.volumeMul;

        // Decay time.
        accum[SRD_DECAY]   += envSpaceAccum[i] * envInfo.decayMul;

        // High frequency damping.
        accum[SRD_DAMPING] += envSpaceAccum[i] * envInfo.dampingMul;
    }
    d->reverb[SRD_VOLUME]  = de::min(accum[SRD_VOLUME],  255);
    d->reverb[SRD_DECAY]   = de::min(accum[SRD_DECAY],   255);
    d->reverb[SRD_DAMPING] = de::min(accum[SRD_DAMPING], 255);

    return true;
}

BspLeaf::AudioEnvironmentFactors const &BspLeaf::reverb() const
{
    return d->reverb;
}

void BspLeaf::clearShadowLines()
{
    d->shadowLines.clear();
}

void BspLeaf::addShadowLine(LineSide &side)
{
    if(!hasPoly()) return;
    d->shadowLines.insert(&side);
}

BspLeaf::ShadowLines const &BspLeaf::shadowLines() const
{
    return d->shadowLines;
}

void BspLeaf::unlinkAllLumobjs()
{
    d->lumobjs.clear();
}

void BspLeaf::unlink(Lumobj &lumobj)
{
    d->lumobjs.remove(&lumobj);
}

void BspLeaf::link(Lumobj &lumobj)
{
    if(!hasPoly()) return;
    d->lumobjs.insert(&lumobj);
}

BspLeaf::Lumobjs const &BspLeaf::lumobjs() const
{
    return d->lumobjs;
}

int BspLeaf::lastSpriteProjectFrame() const
{
    return d->addSpriteCount;
}

void BspLeaf::setLastSpriteProjectFrame(int newFrameCount)
{
    d->addSpriteCount = newFrameCount;
}

#endif // __CLIENT__
