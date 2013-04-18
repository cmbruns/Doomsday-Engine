/** @file edit_bsp.cpp BSP Builder. 
 * @ingroup map
 *
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

#include <cstdlib>
#include <cmath>

#include "de_base.h"
#include "de_bsp.h"
#include "de_console.h"
#include "de_misc.h"
#include "de_play.h"

#include <de/Log>
#include <BspBuilder>

using namespace de;

int bspFactor = 7;

struct bspbuilder_c_s {
   BspBuilder* inst;
};

void BspBuilder_Register(void)
{
    C_VAR_INT("bsp-factor", &bspFactor, CVF_NO_MAX, 0, 0);
}

BspBuilder_c* BspBuilder_New(GameMap* map, uint numEditableVertexes, Vertex const **editableVertexes)
{
    DENG2_ASSERT(map);
    BspBuilder_c* builder = static_cast<BspBuilder_c*>(malloc(sizeof *builder));
    builder->inst = new BspBuilder(*map, numEditableVertexes, editableVertexes);
    return builder;
}

void BspBuilder_Delete(BspBuilder_c* builder)
{
    DENG2_ASSERT(builder);
    delete builder->inst;
    free(builder);
}

BspBuilder_c* BspBuilder_SetSplitCostFactor(BspBuilder_c* builder, int factor)
{
    DENG2_ASSERT(builder);
    builder->inst->setSplitCostFactor(factor);
    return builder;
}

boolean BspBuilder_Build(BspBuilder_c* builder)
{
    DENG2_ASSERT(builder);
    return CPP_BOOL(builder->inst->build());
}

typedef struct {
    BspBuilder* builder;
    size_t curIdx;
    HEdge*** hedgeLUT;
} hedgecollectorparams_t;

static int hedgeCollector(BspTreeNode& tree, void* parameters)
{
    if(tree.isLeaf())
    {
        hedgecollectorparams_t* p = static_cast<hedgecollectorparams_t*>(parameters);
        BspLeaf* leaf = tree.userData()->castTo<BspLeaf>();
        HEdge* hedge = leaf->hedge;
        do
        {
            // Take ownership of this HEdge.
            p->builder->take(hedge);

            // Add this HEdge to the LUT.
            hedge->index = p->curIdx++;
            (*p->hedgeLUT)[hedge->index] = hedge;

        } while((hedge = hedge->next) != leaf->hedge);
    }
    return false; // Continue traversal.
}

static void buildHEdgeLut(BspBuilder& builder, GameMap* map)
{
    DENG2_ASSERT(map);

    if(map->hedges)
    {
        Z_Free(map->hedges);
        map->hedges = 0;
    }

    map->numHEdges = builder.numHEdges();
    if(!map->numHEdges) return; // Should never happen.

    // Allocate the LUT and acquire ownership of the half-edges.
    map->hedges = static_cast<HEdge**>(Z_Calloc(map->numHEdges * sizeof(HEdge*), PU_MAPSTATIC, 0));

    hedgecollectorparams_t parm;
    parm.builder = &builder;
    parm.curIdx = 0;
    parm.hedgeLUT = &map->hedges;
    builder.root()->traverseInOrder(hedgeCollector, &parm);
}

static void finishHEdges(GameMap* map)
{
    DENG2_ASSERT(map);

    for(uint i = 0; i < map->numHEdges; ++i)
    {
        HEdge* hedge = map->hedges[i];

        if(hedge->lineDef)
        {
            Vertex *vtx = hedge->lineDef->L_v(hedge->side);

            hedge->sector = hedge->lineDef->L_sector(hedge->side);
            hedge->offset = V2d_Distance(hedge->HE_v1origin, vtx->origin);
        }

        hedge->angle = bamsAtan2((int) (hedge->HE_v2origin[VY] - hedge->HE_v1origin[VY]),
                                 (int) (hedge->HE_v2origin[VX] - hedge->HE_v1origin[VX])) << FRACBITS;

        // Calculate the length of the segment.
        hedge->length = V2d_Distance(hedge->HE_v2origin, hedge->HE_v1origin);

        if(hedge->length == 0)
            hedge->length = 0.01f; // Hmm...
    }
}

typedef struct {
    BspBuilder* builder;
    GameMap* dest;
    uint leafCurIndex;
    uint nodeCurIndex;
} populatebspobjectluts_params_t;

static int populateBspObjectLuts(BspTreeNode& tree, void* parameters)
{
    populatebspobjectluts_params_t* p = static_cast<populatebspobjectluts_params_t*>(parameters);
    DENG2_ASSERT(p);

    // We are only interested in BspNodes at this level.
    if(tree.isLeaf()) return false; // Continue iteration.

    // Take ownership of this BspNode.
    DENG2_ASSERT(tree.userData());
    BspNode* node = tree.userData()->castTo<BspNode>();
    p->builder->take(node);

    // Add this BspNode to the LUT.
    node->index = p->nodeCurIndex++;
    p->dest->bspNodes[node->index] = node;

    if(BspTreeNode* right = tree.right())
    {
        if(right->isLeaf())
        {
            // Take ownership of this BspLeaf.
            DENG2_ASSERT(right->userData());
            BspLeaf* leaf = right->userData()->castTo<BspLeaf>();
            p->builder->take(leaf);

            // Add this BspLeaf to the LUT.
            leaf->index = p->leafCurIndex++;
            p->dest->bspLeafs[leaf->index] = leaf;
        }
    }

    if(BspTreeNode* left = tree.left())
    {
        if(left->isLeaf())
        {
            // Take ownership of this BspLeaf.
            DENG2_ASSERT(left->userData());
            BspLeaf* leaf = left->userData()->castTo<BspLeaf>();
            p->builder->take(leaf);

            // Add this BspLeaf to the LUT.
            leaf->index = p->leafCurIndex++;
            p->dest->bspLeafs[leaf->index] = leaf;
        }
    }

    return false; // Continue iteration.
}

static void hardenBSP(BspBuilder& builder, GameMap* dest)
{
    dest->numBspNodes = builder.numNodes();
    if(dest->numBspNodes != 0)
        dest->bspNodes = static_cast<BspNode**>(Z_Malloc(dest->numBspNodes * sizeof(BspNode*), PU_MAPSTATIC, 0));
    else
        dest->bspNodes = 0;

    dest->numBspLeafs = builder.numLeafs();
    dest->bspLeafs = static_cast<BspLeaf**>(Z_Calloc(dest->numBspLeafs * sizeof(BspLeaf*), PU_MAPSTATIC, 0));

    BspTreeNode* rootNode = builder.root();
    dest->bsp = rootNode->userData();

    if(rootNode->isLeaf())
    {
        // Take ownership of this leaf.
        DENG2_ASSERT(rootNode->userData());
        BspLeaf* leaf = rootNode->userData()->castTo<BspLeaf>();
        builder.take(leaf);

        // Add this BspLeaf to the LUT.
        leaf->index = 0;
        dest->bspLeafs[0] = leaf;

        return;
    }

    populatebspobjectluts_params_t p;
    p.builder = &builder;
    p.dest = dest;
    p.leafCurIndex = 0;
    p.nodeCurIndex = 0;
    rootNode->traversePostOrder(populateBspObjectLuts, &p);
}

static void copyVertex(Vertex& dest, Vertex const& src)
{
    DENG2_ASSERT(dest.type() == DMU_VERTEX);

    V2d_Copy(dest.origin, src.origin);
    dest.numLineOwners = src.numLineOwners;
    dest.lineOwners = src.lineOwners;

    dest.buildData.index = src.buildData.index;
    dest.buildData.refCount = src.buildData.refCount;
    dest.buildData.equiv = src.buildData.equiv;
}

static void hardenVertexes(BspBuilder& builder, GameMap* map,
    uint* numEditableVertexes, Vertex const ***editableVertexes)
{
    uint bspVertexCount = builder.numVertexes();

    map->vertexes.clearAndResize(*numEditableVertexes + bspVertexCount);

    //map->vertexes = static_cast<Vertex*>(Z_Calloc(map->numVertexes * sizeof(Vertex), PU_MAPSTATIC, 0));

    uint n = 0;
    for(; n < *numEditableVertexes; ++n)
    {
        copyVertex(map->vertexes[n], *((*editableVertexes)[n]));
    }

    for(uint i = 0; i < bspVertexCount; ++i, ++n)
    {
        Vertex& dest = map->vertexes[n];
        Vertex& src  = builder.vertex(i);

        builder.take(&src);

        copyVertex(dest, src);
    }
}

static void updateVertexLinks(GameMap* map)
{
    for(uint i = 0; i < map->lineDefCount(); ++i)
    {
        LineDef* line = &map->lineDefs[i];

        line->L_v1 = &map->vertexes[line->L_v1->buildData.index - 1];
        line->L_v2 = &map->vertexes[line->L_v2->buildData.index - 1];
    }

    for(uint i = 0; i < map->numHEdges; ++i)
    {
        HEdge* hedge = map->hedges[i];

        hedge->HE_v1 = &map->vertexes[hedge->v[0]->buildData.index - 1];
        hedge->HE_v2 = &map->vertexes[hedge->v[1]->buildData.index - 1];
    }
}

void MPE_SaveBsp(BspBuilder_c* builder_c, GameMap* map, uint numEditableVertexes,
                 Vertex const **editableVertexes)
{
    DENG2_ASSERT(builder_c);
    BspBuilder& builder = *builder_c->inst;

    dint32 rHeight, lHeight;
    BspTreeNode* rootNode = builder.root();
    if(!rootNode->isLeaf())
    {
        rHeight = dint32(rootNode->right()->height());
        lHeight = dint32(rootNode->left()->height());
    }
    else
    {
        rHeight = lHeight = 0;
    }

    LOG_INFO("BSP built: (%d:%d) %d Nodes, %d Leafs, %d HEdges, %d Vertexes.")
            << rHeight << lHeight << builder.numNodes() << builder.numLeafs()
            << builder.numHEdges() << builder.numVertexes();

    buildHEdgeLut(builder, map);
    hardenVertexes(builder, map, &numEditableVertexes, &editableVertexes);
    updateVertexLinks(map);

    finishHEdges(map);
    hardenBSP(builder, map);
}