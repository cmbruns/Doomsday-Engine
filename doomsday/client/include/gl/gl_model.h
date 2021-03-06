/** @file gl_model.h  MD2 and DMD2 3D model formats
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

#ifndef LIBDENG_GL_MODEL_H
#define LIBDENG_GL_MODEL_H

#include "dd_types.h"
#include "resource/r_data.h"
#include "resource/texture.h"

#define MD2_MAGIC           0x32504449
#define NUMVERTEXNORMALS    162
//#define MAX_MODELS          768

// "DMDM" = Doomsday/Detailed MoDel Magic
#define DMD_MAGIC           0x4D444D44
#define MAX_LODS            4

typedef struct {
    int             magic;
    int             version;
    int             skinWidth;
    int             skinHeight;
    int             frameSize;
    int             numSkins;
    int             numVertices;
    int             numTexCoords;
    int             numTriangles;
    int             numGlCommands;
    int             numFrames;
    int             offsetSkins;
    int             offsetTexCoords;
    int             offsetTriangles;
    int             offsetFrames;
    int             offsetGlCommands;
    int             offsetEnd;
} md2_header_t;

typedef struct {
    byte            vertex[3];
    byte            lightNormalIndex;
} md2_triangleVertex_t;

typedef struct {
    float           scale[3];
    float           translate[3];
    char            name[16];
    md2_triangleVertex_t vertices[1];
} md2_packedFrame_t;

typedef struct {
    float           vertex[3];
    byte            lightNormalIndex;
} md2_modelVertex_t;

// Translated frame (vertices in model space).
typedef struct {
    char            name[16];
    md2_modelVertex_t *vertices;
} md2_frame_t;

typedef struct {
    char            name[256];
    int             id;
} md2_skin_t;

typedef struct {
    short           vertexIndices[3];
    short           textureIndices[3];
} md2_triangle_t;

typedef struct {
    short           s, t;
} md2_textureCoordinate_t;

typedef struct {
    float           s, t;
    int             vertexIndex;
} md2_glCommandVertex_t;

//===========================================================================
// DMD (Detailed/Doomsday Models)
//===========================================================================

typedef struct {
    int magic;
    int version;
    int flags;
} dmd_header_t;

// Chunk types.
enum {
    DMC_END, /// Must be the last chunk.
    DMC_INFO /// Required; will be expected to exist.
};

#pragma pack(1)
typedef struct {
    int type;
    int length; /// Next chunk follows...
} dmd_chunk_t;

typedef struct {
    int skinWidth;
    int skinHeight;
    int frameSize;
    int numSkins;
    int numVertices;
    int numTexCoords;
    int numFrames;
    int numLODs;
    int offsetSkins;
    int offsetTexCoords;
    int offsetFrames;
    int offsetLODs;
    int offsetEnd;
} dmd_info_t;

typedef struct {
    int numTriangles;
    int numGlCommands;
    int offsetTriangles;
    int offsetGlCommands;
} dmd_levelOfDetail_t;

typedef struct {
    byte vertex[3];
    unsigned short normal; /// Yaw and pitch.
} dmd_packedVertex_t;

typedef struct {
    float scale[3];
    float translate[3];
    char name[16];
    dmd_packedVertex_t vertices[1];
} dmd_packedFrame_t;

typedef struct {
    char name[256];
    de::Texture *texture;
} dmd_skin_t;

typedef struct {
    short vertexIndices[3];
    short textureIndices[3];
} dmd_triangle_t;

typedef struct {
    float s, t;
    int vertexIndex;
} dmd_glCommandVertex_t;

typedef struct {
    int* glCommands;
} dmd_lod_t;
#pragma pack()

typedef struct model_vertex_s {
    float xyz[3];
} model_vertex_t;

typedef struct model_frame_s {
    char name[16];
    model_vertex_t *vertices;
    model_vertex_t *normals;
    float min[3], max[3];

#ifdef __cplusplus
    void getBounds(float min[3], float max[3]) const;

    float horizontalRange(float *top, float *bottom) const;
#endif
} model_frame_t;

typedef struct model_s {
    uint modelId; ///< Id of the model in the repository.
    dmd_header_t header;
    dmd_info_t info;
    dmd_skin_t *skins;
    model_frame_t *frames;
    dmd_levelOfDetail_t lodInfo[MAX_LODS];
    dmd_lod_t lods[MAX_LODS];
    char *vertexUsage; ///< Bitfield for each vertex.
    boolean allowTexComp; ///< Allow texture compression with this.

#ifdef __cplusplus
    bool validFrameNumber(int value) const;

    model_frame_t &frame(int number) const;

    int frameNumForName(char const *name) const;
#endif
} model_t;

#endif /* LIBDENG_GL_MODEL_H */
