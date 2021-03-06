/** @file sector.h World map sector.
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

#ifndef DENG_WORLD_SECTOR_H
#define DENG_WORLD_SECTOR_H

#include <QList>

#include <de/aabox.h>

#include <de/libdeng2.h>
#include <de/Error>
#include <de/Observers>
#include <de/Vector>

#include "dd_share.h" // AudioEnvironmentFactors

#include "HEdge"

#include "MapElement"
#include "Line"
#include "Plane"

#ifdef __CLIENT__
#  include "render/lightgrid.h"
#endif

class BspLeaf;
class Surface;
struct mobj_s;

/**
 * World map sector.
 *
 * @ingroup world
 */
class Sector : public de::MapElement
{
    DENG2_NO_COPY  (Sector)
    DENG2_NO_ASSIGN(Sector)

public:
    /// Required/referenced plane is missing. @ingroup errors
    DENG2_ERROR(MissingPlaneError);

    /*
     * Notified whenever a light level change occurs.
     */
    DENG2_DEFINE_AUDIENCE(LightLevelChange,
        void sectorLightLevelChanged(Sector &sector, float oldLightLevel))

    /*
     * Notified whenever a light color change occurs.
     */
    DENG2_DEFINE_AUDIENCE(LightColorChange,
        void sectorLightColorChanged(Sector &sector, de::Vector3f const &oldLightColor,
                                     int changedComponents /*bit-field (0x1=Red, 0x2=Green, 0x4=Blue)*/))

    /**
     * Adjacent BSP leafs in the sector (i.e., those which share one or more
     * common edge) are grouped into a "cluster". Clusters are never empty and
     * will always contain at least one BSP leaf.
     *
     * Call Sector::buildClusters to rebuild the cluster set for the sector.
     */
    class Cluster
    {
    public:
        /*
         * Notified when the cluster is about to be deleted.
         */
        DENG2_DEFINE_AUDIENCE(Deletion, void sectorClusterBeingDeleted(Cluster const &cluster))

        typedef QList<BspLeaf *> BspLeafs;

    public:
        /**
         * Construct a new sector cluster comprised of the specified set of BSP
         * leafs. It is assumed that all BSP leafs in the list are attributed to
         * the same sector and there is always at least one.
         *
         * @param bspLeafs  Set of BSP leafs comprising the resulting cluster.
         */
        Cluster(BspLeafs const &bspLeafs);

        /**
         * Determines whether the specified @a hedge is an "internal" edge:
         *
         * - both the half-edge and it's twin have a face.
         * - both faces are assigned to a BSP leaf.
         * - both of the assigned BSP leafs are in the same cluster.
         *
         * @param hedge  Half-edge to test.
         *
         * @return  @c true= @a hedge is a cluster internal edge.
         */
        static bool isInternalEdge(de::HEdge *hedge);

        /**
         * Returns the parent sector of the cluster.
         */
        Sector &sector() const;

        /**
         * Returns the identified @em physical plane of the parent sector. Note
         * that this is not the same as the "visual" plane which may well be
         * defined by another sector.
         *
         * @param planeIndex  Index of the plane to return.
         */
        Plane &plane(int planeIndex) const;

        /**
         * Returns the sector plane which defines the @em physical floor of the
         * cluster.
         * @see hasSector(), plane()
         */
        inline Plane &floor() const { return plane(Sector::Floor); }

        /**
         * Returns the sector plane which defines the @em physical ceiling of
         * the cluster.
         * @see hasSector(), plane()
         */
        inline Plane &ceiling() const { return plane(Sector::Ceiling); }

        /**
         * Returns the identified @em visual sector plane for the cluster (which
         * may or may not be the same as the physical plane).
         *
         * @param planeIndex  Index of the plane to return.
         */
        Plane &visPlane(int planeIndex) const;

        /**
         * Returns the sector plane which defines the @em visual floor of the
         * cluster.
         * @see hasSector(), floor()
         */
        inline Plane &visFloor() const   { return visPlane(Sector::Floor); }

        /**
         * Returns the sector plane which defines the @em visual ceiling of the
         * cluster.
         * @see hasSector(), ceiling()
         */
        inline Plane &visCeiling() const { return visPlane(Sector::Ceiling); }

        /**
         * Returns the total number of @em visual planes in the cluster.
         */
        inline int visPlaneCount() const { return sector().planeCount(); }

        /**
         * To be called to force re-evaluation of mapped visual planes. This is
         * only necessary when a surface material change occurs on boundary line
         * of the cluster.
         */
        void markVisPlanesDirty();

        /**
         * Returns @c true iff at least one of the mapped visual planes of the
         * cluster presently has a sky-masked material bound.
         *
         * @see Surface::hasSkyMaskedMaterial()
         */
        bool hasSkyMaskedPlane() const;

        /**
         * Provides access to the list of all BSP leafs in the cluster, for
         * efficient traversal.
         */
        BspLeafs const &bspLeafs() const;

        /**
         * Returns the total number of BSP leafs in the cluster.
         */
        inline int bspLeafCount() const { return bspLeafs().count(); }

        /**
         * Returns the axis-aligned bounding box of the cluster.
         */
        AABoxd const &aaBox() const;

        /**
         * Returns the point defined by the center of the axis-aligned bounding
         * box in the map coordinate space.
         */
        inline de::Vector2d center() const {
            return (de::Vector2d(aaBox().min) + de::Vector2d(aaBox().max)) / 2;
        }

#ifdef __CLIENT__

        /**
         * Determines whether the cluster has a positive world volume, i.e., the
         * height of floor is lower than that of the ceiling plane.
         *
         * @param useSmoothedHeights  @c true= use the @em smoothed plane heights
         *                            instead of the @em sharp heights.
         */
        bool hasWorldVolume(bool useSmoothedHeights = true) const;

        /**
         * Returns a rough approximation of the total combined area of the geometry
         * for all BSP leafs which define the cluster (map units squared).
         */
        coord_t roughArea() const;

        /**
         * Request re-calculation of environmental audio (reverb) characteristics
         * for the cluster (update is deferred until next accessed).
         *
         * To be called whenever any of the properties governing reverb properties
         * have changed (i.e., wall/plane material changes).
         */
        void markReverbDirty(bool yes = true);

        /**
         * Returns the final environmental audio characteristics (reverb) of the
         * cluster. Note that if a reverb update is scheduled it will be done at
         * this time (@ref markReverbDirty()).
         */
        AudioEnvironmentFactors const &reverb() const;

#endif // __CLIENT__

    private:
        DENG2_PRIVATE(d)
    };

#ifdef __CLIENT__
    /**
     * LightGrid data values for "smoothed sector lighting".
     *
     * @todo Encapsulate in LightGrid itself?
     */
    struct LightGridData
    {
        /// Number of blocks attributed to the sector.
        uint blockCount;

        /// Number of attributed blocks to mark changed.
        uint changedBlockCount;

        /// Block indices.
        de::LightGrid::Index *blocks;
    };
#endif

    /*
     * Linked-element lists:
     */
    typedef QList<Cluster *>  Clusters;
    typedef QList<Plane *>    Planes;
    typedef QList<LineSide *> Sides;

    // Plane identifiers:
    enum { Floor, Ceiling };

public:
    /**
     * Construct a new sector.
     *
     * @param lightLevel  Ambient light level.
     * @param lightColor  Ambient light color.
     */
    Sector(float lightLevel               = 1,
           de::Vector3f const &lightColor = de::Vector3f(1, 1, 1));

    /**
     * Returns the sector plane with the specified @a planeIndex.
     */
    Plane &plane(int planeIndex);

    /// @copydoc plane()
    Plane const &plane(int planeIndex) const;

    /**
     * Returns the floor plane of the sector.
     */
    inline Plane &floor() { return plane(Floor); }

    /// @copydoc floor()
    inline Plane const &floor() const { return plane(Floor); }

    /**
     * Returns the ceiling plane of the sector.
     */
    inline Plane &ceiling() { return plane(Ceiling); }

    /// @copydoc ceiling()
    inline Plane const &ceiling() const { return plane(Ceiling); }

    Plane *addPlane(de::Vector3f const &normal, coord_t height);

    /**
     * Provides access to the list of planes in/owned by the sector, for efficient
     * traversal.
     */
    Planes const &planes() const;

    /**
     * Returns the total number of planes in/owned by the sector.
     */
    inline int planeCount() const { return planes().count(); }

    /**
     * Convenient accessor method for returning the surface of the specified
     * plane of the sector.
     */
    inline Surface &planeSurface(int planeIndex) { return plane(planeIndex).surface(); }

    /// @copydoc planeSurface()
    inline Surface const &planeSurface(int planeIndex) const { return plane(planeIndex).surface(); }

    /**
     * Convenient accessor method for returning the surface of the floor plane
     * of the sector.
     */
    inline Surface &floorSurface() { return floor().surface(); }

    /// @copydoc floorSurface()
    inline Surface const &floorSurface() const { return floor().surface(); }

    /**
     * Convenient accessor method for returning the surface of the ceiling plane
     * of the sector.
     */
    inline Surface &ceilingSurface() { return ceiling().surface(); }

    /// @copydoc ceilingSurface()
    inline Surface const &ceilingSurface() const { return ceiling().surface(); }

    /**
     * Provides access to the list of line sides which reference the sector,
     * for efficient traversal.
     */
    Sides const &sides() const;

    /**
     * Returns the total number of line sides which reference the sector.
     */
    inline int sideCount() const { return sides().count(); }

    /**
     * (Re)Build the side list for the sector.
     *
     * @note In the special case of self-referencing line, only the front side
     * reference is added to this list.
     *
     * @attention The behavior of some algorithms used in the DOOM game logic
     * is dependant upon the order of this list. For example, EV_DoFloor and
     * EV_BuildStairs. That same order is used here, for compatibility.
     *
     * Order: Original @em line index, ascending.
     */
    void buildSides();

    /**
     * Provides access to the list of BSP leaf clusters for the sector, for
     * efficient traversal.
     */
    Clusters const &clusters() const;

    /**
     * Returns the total number of BSP leaf clusters for the sector.
     */
    inline int clusterCount() const { return clusters().count(); }

    /**
     * (Re)Build BSP leaf clusters for the sector.
     */
    void buildClusters();

    /**
     * Returns the primary sound emitter for the sector. Other emitters in the
     * sector are linked to this, forming a chain which can be traversed using
     * the 'next' pointer of the emitter's thinker_t.
     */
    SoundEmitter &soundEmitter();

    /// @copydoc soundEmitter()
    SoundEmitter const &soundEmitter() const;

    /**
     * (Re)Build the sound emitter chains for the sector. These chains are used
     * for efficiently traversing all sound emitters in the sector (e.g., when
     * stopping all sounds emitted in the sector). To be called during map load
     * once planes and sides have been initialized.
     *
     * @see addPlane(), buildSides()
     */
    void chainSoundEmitters();

    /**
     * Returns the ambient light level in the sector. The LightLevelChange
     * audience is notified whenever the light level changes.
     *
     * @see setLightLevel()
     */
    float lightLevel() const;

    /**
     * Change the ambient light level in the sector. The LightLevelChange
     * audience is notified whenever the light level changes.
     *
     * @param newLightLevel  New ambient light level.
     *
     * @see lightLevel()
     */
    void setLightLevel(float newLightLevel);

    /**
     * Returns the ambient light color in the sector. The LightColorChange
     * audience is notified whenever the light color changes.
     *
     * @see setLightColor(), lightColorComponent(), lightRed(), lightGreen(), lightBlue()
     */
    de::Vector3f const &lightColor() const;

    /**
     * Returns the strength of the specified @a component of the ambient light
     * color in the sector. The LightColorChange audience is notified whenever
     * the light color changes.
     *
     * @param component    RGB index of the color component (0=Red, 1=Green, 2=Blue).
     *
     * @see lightColor(), lightRed(), lightGreen(), lightBlue()
     */
    inline float lightColorComponent(int component) const { return lightColor()[component]; }

    /**
     * Returns the strength of the @em red component of the ambient light
     * color in the sector. The LightColorChange audience is notified whenever
     * the light color changes.
     *
     * @see lightColorComponent(), lightGreen(), lightBlue()
     */
    inline float lightRed() const   { return lightColorComponent(0); }

    /**
     * Returns the strength of the @em green component of the ambient light
     * color in the sector. The LightColorChange audience is notified whenever
     * the light color changes.
     *
     * @see lightColorComponent(), lightRed(), lightBlue()
     */
    inline float lightGreen() const { return lightColorComponent(1); }

    /**
     * Returns the strength of the @em blue component of the ambient light
     * color in the sector. The LightColorChange audience is notified whenever
     * the light color changes.
     *
     * @see lightColorComponent(), lightRed(), lightGreen()
     */
    inline float lightBlue() const  { return lightColorComponent(2); }

    /**
     * Change the ambient light color in the sector. The LightColorChange
     * audience is notified whenever the light color changes.
     *
     * @param newLightColor  New ambient light color.
     *
     * @see lightColor(), setLightColorComponent(), setLightRed(), setLightGreen(), setLightBlue()
     */
    void setLightColor(de::Vector3f const &newLightColor);

    /**
     * Change the strength of the specified @a component of the ambient light
     * color in the sector. The LightColorChange audience is notified whenever
     * the light color changes.
     *
     * @param component    RGB index of the color component (0=Red, 1=Green, 2=Blue).
     * @param newStrength  New strength factor for the color component.
     *
     * @see setLightColor(), setLightRed(), setLightGreen(), setLightBlue()
     */
    void setLightColorComponent(int component, float newStrength);

    /**
     * Change the strength of the red component of the ambient light color in
     * the sector. The LightColorChange audience is notified whenever the light
     * color changes.
     *
     * @param newStrength  New red strength for the ambient light color.
     *
     * @see setLightColorComponent(), setLightGreen(), setLightBlue()
     */
    inline void setLightRed(float newStrength)  { setLightColorComponent(0, newStrength); }

    /**
     * Change the strength of the green component of the ambient light color in
     * the sector. The LightColorChange audience is notified whenever the light
     * color changes.
     *
     * @param newStrength  New green strength for the ambient light color.
     *
     * @see setLightColorComponent(), setLightRed(), setLightBlue()
     */
    inline void setLightGreen(float newStrength) { setLightColorComponent(1, newStrength); }

    /**
     * Change the strength of the blue component of the ambient light color in
     * the sector. The LightColorChange audience is notified whenever the light
     * color changes.
     *
     * @param newStrength  New blue strength for the ambient light color.
     *
     * @see setLightColorComponent(), setLightRed(), setLightGreen()
     */
    inline void setLightBlue(float newStrength)  { setLightColorComponent(2, newStrength); }

    /**
     * Returns the first mobj in the linked list of mobjs "in" the sector.
     */
    struct mobj_s *firstMobj() const;

    /**
     * Unlink the mobj from the list of mobjs "in" the sector.
     *
     * @param mobj  Mobj to be unlinked.
     */
    void unlink(struct mobj_s *mobj);

    /**
     * Link the mobj to the head of the list of mobjs "in" the sector. Note that
     * mobjs in this list may not actually be inside the sector. This is because
     * the sector is determined by interpreting the BSP leaf as a half-space and
     * not a closed convex subspace (@ref Map::link()).
     *
     * @param mobj  Mobj to be linked.
     */
    void link(struct mobj_s *mobj);

    /**
     * Returns the @em validCount of the sector. Used by some legacy iteration
     * algorithms for marking sectors as processed/visited.
     *
     * @todo Refactor away.
     */
    int validCount() const;

    /// @todo Refactor away.
    void setValidCount(int newValidCount);

#ifdef __CLIENT__

    /**
     * Returns the axis-aligned bounding box which encompases the geometry of
     * all BSP leafs attributed to the sector (map units squared). Note that if
     * no BSP leafs reference the sector the bounding box will be invalid (has
     * negative dimensions).
     *
     * @todo Refactor away (still used by light decoration and particle systems).
     */
    AABoxd const &aaBox() const;

    /**
     * Returns a rough approximation of the total combined area of the geometry
     * for all BSP leafs attributed to the sector (map units squared).
     *
     * @todo Refactor away (still used by the particle system).
     */
    coord_t roughArea() const;

    /**
     * Returns the LightGrid data values (for smoothed ambient lighting) for
     * the sector.
     */
    LightGridData &lightGridData();

#endif // __CLIENT__

protected:
    int property(DmuArgs &args) const;
    int setProperty(DmuArgs const &args);

private:
    DENG2_PRIVATE(d)
};

typedef Sector::Cluster SectorCluster;

/**
 * Specialized sector cluster half-edge circulator. Used like an iterator, for
 * circumnavigating the boundary half-edges of a cluster.
 *
 * Cluster-internal edges (i.e., where both half-edge faces reference the same
 * cluster) are automatically skipped during traversal. Otherwise behavior is
 * the same as a "regular" half-edge face circulator.
 *
 * Also provides static search utilities for convenient, one-time use of this
 * specialized search logic (avoiding circulator instantiation).
 */
class SectorClusterCirculator
{
public:
    /// Attempt to dereference a NULL circulator. @ingroup errors
    DENG2_ERROR(NullError);

public:
    /**
     * Construct a new sector cluster circulator.
     *
     * @param hedge  Half-edge to circulate. It is assumed the half-edge lies on
     * the @em boundary of the cluster and is not an "internal" edge.
     */
    SectorClusterCirculator(de::HEdge *hedge = 0)
        : _hedge(hedge),
          _current(hedge),
          _cluster(hedge? getCluster(*hedge) : 0)
    {}

    /**
     * Intended as a convenient way to employ the specialized circulator logic
     * to locate the relative back of the next/previous neighboring half-edge.
     * Particularly useful when a geometry traversal requires a switch from the
     * cluster to face boundary, or when navigating the so-called "one-ring" of
     * a vertex.
     */
    static de::HEdge &findBackNeighbor(de::HEdge &hedge, de::ClockDirection direction)
    {
        return getNeighbor(hedge, direction, getCluster(hedge)).twin();
    }

    /**
     * Returns the neighbor half-edge in the specified @a direction around the
     * boundary of the cluster.
     *
     * @param direction  Relative direction of the desired neighbor.
     */
    de::HEdge &neighbor(de::ClockDirection direction) {
        _current = &getNeighbor(*_current, direction, _cluster);
        return *_current;
    }

    /// Returns the next half-edge (clockwise) and advances the circulator.
    inline de::HEdge &next() { return neighbor(de::Clockwise); }

    /// Returns the previous half-edge (anticlockwise) and advances the circulator.
    inline de::HEdge &previous() { return neighbor(de::Anticlockwise); }

    /// Advance to the next half-edge (clockwise).
    inline SectorClusterCirculator &operator ++ () {
        next(); return *this;
    }
    /// Advance to the previous half-edge (anticlockwise).
    inline SectorClusterCirculator &operator -- () {
        previous(); return *this;
    }

    /// Returns @c true iff @a other references the same half-edge as "this"
    /// circulator; otherwise returns false.
    inline bool operator == (SectorClusterCirculator const &other) const {
        return _current == other._current;
    }
    inline bool operator != (SectorClusterCirculator const &other) const {
        return !(*this == other);
    }

    /// Returns @c true iff the range of the circulator [c, c) is not empty.
    inline operator bool () const { return _hedge != 0; }

    /// Makes the circulator operate on @a hedge.
    SectorClusterCirculator &operator = (de::HEdge &hedge) {
        _hedge = _current = &hedge;
        _cluster = getCluster(hedge);
        return *this;
    }

    /// Returns the current half-edge of a non-empty sequence.
    de::HEdge &operator * () const {
        if(!_current)
        {
            /// @throw NullError Attempted to dereference a "null" circulator.
            throw NullError("SectorClusterCirculator::operator *", "Circulator references an empty sequence");
        }
        return *_current;
    }

    /// Returns a pointer to the current half-edge (might be @c NULL, meaning the
    /// circulator references an empty sequence).
    de::HEdge *operator -> () const { return _current; }

private:
    static SectorCluster *getCluster(de::HEdge &hedge);

    static de::HEdge &getNeighbor(de::HEdge &hedge, de::ClockDirection direction,
                                  SectorCluster *cluster = 0);

    de::HEdge *_hedge;
    de::HEdge *_current;
    SectorCluster *_cluster;
};

#endif // DENG_WORLD_SECTOR_H
