/* Generated by ../../engine/scripts/makedmt.py */

#ifndef __DOOMSDAY_PLAY_MAP_DATA_TYPES_H__
#define __DOOMSDAY_PLAY_MAP_DATA_TYPES_H__

#include "p_mapdata.h"

typedef struct vertex_s {
    runtime_mapdata_header_t header;
    float               pos[2];
    unsigned int        numsecowners;  // Number of sector owners.
    unsigned int*       secowners;     // Sector indices [numsecowners] size.
    unsigned int        numlineowners; // Number of line owners.
    struct lineowner_s* lineowners;    // Lineowner base ptr [numlineowners] size. A doubly, circularly linked list. The base is the line with the lowest angle and the next-most with the largest angle.
    boolean             anchored;      // One or more of our line owners are one-sided.
} vertex_t;

// Helper macros for accessing seg data elements.
#define FRONT 0
#define BACK  1

#define SG_v1                   v[0]
#define SG_v2                   v[1]
#define SG_frontsector          sec[FRONT]
#define SG_backsector           sec[BACK]

// Seg frame flags
#define SEGINF_FACINGFRONT      0x0001
#define SEGINF_BACKSECSKYFIX    0x0002

typedef struct seg_s {
    runtime_mapdata_header_t header;
    struct vertex_s*    v[2];          // [Start, End] of the segment.
    float               length;        // Accurate length of the segment (v1 -> v2).
    fixed_t             offset;
    struct side_s*      sidedef;
    struct line_s*      linedef;
    struct sector_s*    sec[2];
    angle_t             angle;
    byte                flags;
    short               frameflags;
    struct biastracker_s tracker[3];   // 0=top, 1=middle, 2=bottom
    struct vertexillum_s illum[3][4];
    unsigned int        updated;
    struct biasaffection_s affected[MAX_BIAS_AFFECTED];
} seg_t;

typedef struct subsector_s {
    runtime_mapdata_header_t header;
    struct sector_s*    sector;
    unsigned int        linecount;
    unsigned int        firstline;
    struct polyobj_s*   poly;          // NULL, if there is no polyobj.
    byte                flags;
    unsigned short      numverts;
    fvertex_t*          verts;         // A sorted list of edge vertices.
    fvertex_t           bbox[2];       // Min and max points.
    fvertex_t           midpoint;      // Center of vertices.
    struct subplaneinfo_s** planes;
    unsigned short      numvertices;
    struct fvertex_s*   vertices;
    int                 validcount;
    struct shadowlink_s* shadows;
} subsector_t;

// Surface flags.
#define SUF_TEXFIX      0x1         // Current texture is a fix replacement
                                    // (not sent to clients, returned via DMU etc).
#define SUF_GLOW        0x2         // Surface glows (full bright).
#define SUF_BLEND       0x4         // Surface possibly has a blended texture.
#define SUF_NO_RADIO    0x8         // No fakeradio for this surface.

typedef struct surface_s {
    runtime_mapdata_header_t header;
    int                 flags;         // SUF_ flags
    int                 oldflags;
    short               texture;
    short               oldtexture;
    boolean             isflat;        // true if current texture is a flat
    boolean             oldisflat;
    float               normal[3];     // Surface normal
    float               oldnormal[3];
    fixed_t             texmove[2];    // Texture movement X and Y
    fixed_t             oldtexmove[2];
    float               offx;          // Texture x offset
    float               oldoffx;
    float               offy;          // Texture y offset
    float               oldoffy;
    byte                rgba[4];       // Surface color tint
    byte                oldrgba[4];
    struct translation_s* xlat;
} surface_t;

typedef struct plane_s {
    runtime_mapdata_header_t header;
    float               height;        // Current height
    float               oldheight[2];
    surface_t           surface;
    float               glow;          // Glow amount
    byte                glowrgb[3];    // Glow color
    float               target;        // Target height
    float               speed;         // Move speed
    degenmobj_t         soundorg;      // Sound origin for plane
    struct sector_s*    sector;        // Owner of the plane (temp)
    float               visheight;     // Visible plane height (smoothed)
    float               visoffset;
    struct sector_s*    linked;        // Plane attached to another sector.
} plane_t;

// Helper macros for accessing sector floor/ceiling plane data elements.
#define SP_ceilsurface          planes[PLN_CEILING]->surface
#define SP_ceilheight           planes[PLN_CEILING]->height
#define SP_ceilnormal           planes[PLN_CEILING]->normal
#define SP_ceilpic              planes[PLN_CEILING]->surface.texture
#define SP_ceilisflat           planes[PLN_CEILING]->surface.isflat
#define SP_ceiloffx             planes[PLN_CEILING]->surface.offx
#define SP_ceiloffy             planes[PLN_CEILING]->surface.offy
#define SP_ceilrgb              planes[PLN_CEILING]->surface.rgba
#define SP_ceilglow             planes[PLN_CEILING]->glow
#define SP_ceilglowrgb          planes[PLN_CEILING]->glowrgb
#define SP_ceiltarget           planes[PLN_CEILING]->target
#define SP_ceilspeed            planes[PLN_CEILING]->speed
#define SP_ceiltexmove          planes[PLN_CEILING]->surface.texmove
#define SP_ceilsoundorg         planes[PLN_CEILING]->soundorg
#define SP_ceilvisheight        planes[PLN_CEILING]->visheight
#define SP_ceillinked           planes[PLN_CEILING]->linked

#define SP_floorsurface         planes[PLN_FLOOR]->surface
#define SP_floorheight          planes[PLN_FLOOR]->height
#define SP_floornormal          planes[PLN_FLOOR]->normal
#define SP_floorpic             planes[PLN_FLOOR]->surface.texture
#define SP_floorisflat          planes[PLN_FLOOR]->surface.isflat
#define SP_flooroffx            planes[PLN_FLOOR]->surface.offx
#define SP_flooroffy            planes[PLN_FLOOR]->surface.offy
#define SP_floorrgb             planes[PLN_FLOOR]->surface.rgba
#define SP_floorglow            planes[PLN_FLOOR]->glow
#define SP_floorglowrgb         planes[PLN_FLOOR]->glowrgb
#define SP_floortarget          planes[PLN_FLOOR]->target
#define SP_floorspeed           planes[PLN_FLOOR]->speed
#define SP_floortexmove         planes[PLN_FLOOR]->surface.texmove
#define SP_floorsoundorg        planes[PLN_FLOOR]->soundorg
#define SP_floorvisheight       planes[PLN_FLOOR]->visheight
#define SP_floorlinked          planes[PLN_FLOOR]->linked

#define SECT_PLANE_HEIGHT(x, n) (x->planes[n]->visheight)

// Sector frame flags
#define SIF_VISIBLE         0x1    // Sector is visible on this frame.
#define SIF_FRAME_CLEAR     0x1    // Flags to clear before each frame.
#define SIF_LIGHT_CHANGED   0x2

// Sector flags.
#define SECF_INVIS_FLOOR    0x1
#define SECF_INVIS_CEILING  0x2

typedef struct sector_s {
    runtime_mapdata_header_t header;
    short               lightlevel;
    int                 oldlightlevel;
    byte                rgb[3];
    byte                oldrgb[3];
    int                 validcount;    // if == validcount, already checked.
    struct mobj_s*      thinglist;     // List of mobjs in the sector.
    unsigned int        linecount;
    struct line_s**     Lines;         // [linecount] size.
    unsigned int        subscount;
    struct subsector_s** subsectors;   // [subscount] size.
    skyfix_t            skyfix[2];     // floor, ceiling.
    degenmobj_t         soundorg;
    float               reverb[NUM_REVERB_DATA];
    int                 blockbox[4];   // Mapblock bounding box.
    unsigned int        planecount;
    struct plane_s**    planes;        // [planecount] size.
    struct sector_s*    containsector; // Sector that contains this (if any).
    boolean             permanentlink;
    boolean             unclosed;      // An unclosed sector (some sort of fancy hack).
    boolean             selfRefHack;   // A self-referencing hack sector which ISNT enclosed by the sector referenced. Bounding box for the sector.
    float               bounds[4];     // Bounding box for the sector
    int                 frameflags;
    int                 addspritecount; // frame number of last R_AddSprites
    struct sector_s*    lightsource;   // Main sky light source
    unsigned int        blockcount;    // Number of gridblocks in the sector.
    unsigned int        changedblockcount; // Number of blocks to mark changed.
    unsigned short*     blocks;        // Light grid block indices.
} sector_t;

// Parts of a wall segment.
typedef enum segsection_e {
    SEG_MIDDLE,
    SEG_TOP,
    SEG_BOTTOM
} segsection_t;

// Helper macros for accessing sidedef top/middle/bottom section data elements.
#define SW_middlesurface        sections[SEG_MIDDLE]
#define SW_middleflags          sections[SEG_MIDDLE].flags
#define SW_middlepic            sections[SEG_MIDDLE].texture
#define SW_middleisflat         sections[SEG_MIDDLE].isflat
#define SW_middlenormal         sections[SEG_MIDDLE].normal
#define SW_middletexmove        sections[SEG_MIDDLE].texmove
#define SW_middleoffx           sections[SEG_MIDDLE].offx
#define SW_middleoffy           sections[SEG_MIDDLE].offy
#define SW_middlergba           sections[SEG_MIDDLE].rgba
#define SW_middletexlat         sections[SEG_MIDDLE].xlat

#define SW_topsurface           sections[SEG_TOP]
#define SW_topflags             sections[SEG_TOP].flags
#define SW_toppic               sections[SEG_TOP].texture
#define SW_topisflat            sections[SEG_TOP].isflat
#define SW_topnormal            sections[SEG_TOP].normal
#define SW_toptexmove           sections[SEG_TOP].texmove
#define SW_topoffx              sections[SEG_TOP].offx
#define SW_topoffy              sections[SEG_TOP].offy
#define SW_toprgba              sections[SEG_TOP].rgba
#define SW_toptexlat            sections[SEG_TOP].xlat

#define SW_bottomsurface        sections[SEG_BOTTOM]
#define SW_bottomflags          sections[SEG_BOTTOM].flags
#define SW_bottompic            sections[SEG_BOTTOM].texture
#define SW_bottomisflat         sections[SEG_BOTTOM].isflat
#define SW_bottomnormal         sections[SEG_BOTTOM].normal
#define SW_bottomtexmove        sections[SEG_BOTTOM].texmove
#define SW_bottomoffx           sections[SEG_BOTTOM].offx
#define SW_bottomoffy           sections[SEG_BOTTOM].offy
#define SW_bottomrgba           sections[SEG_BOTTOM].rgba
#define SW_bottomtexlat         sections[SEG_BOTTOM].xlat

// Side frame flags
#define SIDEINF_TOPPVIS     0x0001
#define SIDEINF_MIDDLEPVIS  0x0002
#define SIDEINF_BOTTOMPVIS  0x0004

typedef struct side_s {
    runtime_mapdata_header_t header;
    surface_t           sections[3];
    blendmode_t         blendmode;
    struct sector_s*    sector;
    short               flags;
    short               frameflags;
    struct line_s*      neighbor[2];   // Left and right neighbour.
    boolean             pretendneighbor[2]; // If true, neighbor is not a "real" neighbor (it does not share a line with this side's sector).
    struct sector_s*    proxsector[2]; // Sectors behind the neighbors.
    struct line_s*      backneighbor[2]; // Neighbour in the backsector (if any).
    struct line_s*      alignneighbor[2]; // Aligned left and right neighbours.
} side_t;

// Helper macros for accessing linedef data elements.
#define L_v1                    v[0]
#define L_v2                    v[1]
#define L_frontsector           sec[FRONT]
#define L_backsector            sec[BACK]
#define L_frontside             sides[FRONT]
#define L_backside              sides[BACK]

typedef struct line_s {
    runtime_mapdata_header_t header;
    struct vertex_s*    v[2];
    short               flags;
    struct sector_s*    sec[2];        // [front, back] sectors.
    float               dx;
    float               dy;
    slopetype_t         slopetype;
    int                 validcount;
    struct side_s*      sides[2];
    fixed_t             bbox[4];
    float               length;        // Accurate length
    binangle_t          angle;         // Calculated from front side's normal
    boolean             selfrefhackroot; // This line is the root of a self-referencing hack sector
} line_t;

typedef struct polyobj_s {
    runtime_mapdata_header_t header;
    unsigned int        numsegs;
    struct seg_s**      segs;
    int                 validcount;
    degenmobj_t         startSpot;
    angle_t             angle;
    int                 tag;           // reference tag assigned in HereticEd
    ddvertex_t*         originalPts;   // used as the base for the rotations
    ddvertex_t*         prevPts;       // use to restore the old point values
    fixed_t             bbox[4];
    fvertex_t           dest;
    int                 speed;         // Destination XY and speed.
    angle_t             destAngle;     // Destination angle.
    angle_t             angleSpeed;    // Rotation speed.
    boolean             crush;         // should the polyobj attempt to crush mobjs?
    int                 seqType;
    fixed_t             size;          // polyobj size (area of POLY_AREAUNIT == size of FRACUNIT)
    void*               specialdata;   // pointer a thinker, if the poly is moving
} polyobj_t;

typedef struct node_s {
    runtime_mapdata_header_t header;
    fixed_t             x;             // Partition line.
    fixed_t             y;             // Partition line.
    fixed_t             dx;            // Partition line.
    fixed_t             dy;            // Partition line.
    fixed_t             bbox[2][4];    // Bounding box for each child.
    unsigned int        children[2];   // If NF_SUBSECTOR it's a subsector.
} node_t;

#endif
