/** @file rend_particle.cpp Particle Effect Rendering.
 * @ingroup render
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

#include <cstdlib>

#include "de_base.h"
#include "de_console.h"
#include "de_filesys.h"
#include "de_graphics.h"
#include "de_render.h"
#include "de_misc.h"
#include "de_play.h"
#include "de_ui.h"

#include "resource/image.h"
#include "gl/texturecontent.h"

#include "BspLeaf"
#include "Line"
#include "Plane"
#include "world/generators.h"
#include "world/map.h"

using namespace de;

// Point + custom textures.
#define NUM_TEX_NAMES           (MAX_PTC_TEXTURES)

typedef struct {
    Generators::ptcgenid_t ptcGenID; // Generator id.
    int ptID; // Particle id.
    float distance;
} porder_t;

extern float vang, vpitch;

DGLuint pointTex, ptctexname[MAX_PTC_TEXTURES];
int particleNearLimit = 0;
float particleDiffuse = 4;

static size_t numParts;
static boolean hasPoints, hasLines, hasModels, hasNoBlend, hasBlend;
static boolean hasPointTexs[NUM_TEX_NAMES];
static byte visiblePtcGens[Generators::GENERATORS_MAX];

static size_t orderSize = 0;
static porder_t *order = NULL;

// Currently active Generators collection. Global for performance.
static Generators *gens;

void Rend_ParticleRegister()
{
    // Cvars
    C_VAR_BYTE ("rend-particle",                   &useParticles,      0,              0, 1);
    C_VAR_INT  ("rend-particle-max",               &maxParticles,      CVF_NO_MAX,     0, 0);
    C_VAR_FLOAT("rend-particle-rate",              &particleSpawnRate, 0,              0, 5);
    C_VAR_FLOAT("rend-particle-diffuse",           &particleDiffuse,   CVF_NO_MAX,     0, 0);
    C_VAR_INT  ("rend-particle-visible-near",      &particleNearLimit, CVF_NO_MAX,     0, 0);
}

static int markPtcGenVisible(ptcgen_t *gen, void *parameters)
{
    DENG_UNUSED(parameters);

    visiblePtcGens[gens->generatorId(gen)] = true;

    return false; // Continue iteration.
}

static boolean isPtcGenVisible(ptcgen_t const *gen)
{
    return visiblePtcGens[gens->generatorId(gen)];
}

static float pointDist(fixed_t c[3])
{
    viewdata_t const *viewData = R_ViewData(viewPlayer - ddPlayers);
    float dist = ((viewData->current.origin[VY] - FIX2FLT(c[VY])) * -viewData->viewSin) -
        ((viewData->current.origin[VX] - FIX2FLT(c[VX])) * viewData->viewCos);

    if(dist < 0)
        return -dist; // Always return positive.

    return dist;
}

static Path tryFindImage(String name)
{
    /*
     * First look for a colorkeyed version.
     */
    try
    {
        String foundPath = App_FileSystem().findPath(de::Uri("Textures", name + "-ck"),
                                                     RLF_DEFAULT, DD_ResourceClassById(RC_GRAPHIC));
        // Ensure the path is absolute.
        return App_BasePath() / foundPath;
    }
    catch(FS1::NotFoundError const&)
    {} // Ignore this error.

    /*
     * Look for the regular version.
     */
    try
    {
        String foundPath = App_FileSystem().findPath(de::Uri("Textures", name),
                                                     RLF_DEFAULT, DD_ResourceClassById(RC_GRAPHIC));
        // Ensure the path is absolute.
        return App_BasePath() / foundPath;
    }
    catch(FS1::NotFoundError const&)
    {} // Ignore this error.

    return Path(); // Not found.
}

// Try to load the texture.
static byte loadParticleTexture(uint particleTex, bool silent)
{
    DENG_ASSERT(particleTex < MAX_PTC_TEXTURES);

    String particleImageName = String("Particle%1").arg(particleTex, 2, 10, QChar('0'));
    Path foundPath = tryFindImage(particleImageName);
    if(foundPath.isEmpty())
    {
        if(!silent)
        {
            LOG_WARNING("Failed locating image resource for \"%s\".") << particleImageName;
        }
        return 0;
    }

    image_t image;
    if(!GL_LoadImage(image, foundPath.toUtf8().constData()))
        return 0;

    // If 8-bit with no alpha, generate alpha automatically.
    if(image.pixelSize == 1)
        Image_ConvertToAlpha(&image, true);

    // Create a new texture and upload the image.
    ptctexname[particleTex] = GL_NewTextureWithParams(
        image.pixelSize == 4 ? DGL_RGBA :
        image.pixelSize == 2 ? DGL_LUMINANCE_PLUS_A8 : DGL_RGB,
        image.size.width, image.size.height, image.pixels,
        TXCF_NO_COMPRESSION);

    // Free the buffer.
    Image_Destroy(&image);
    return 2; // External
}

void Rend_ParticleLoadSystemTextures()
{
    if(novideo) return;

    if(!pointTex)
    {
        // Load the default "zeroth" texture (a blurred point).
        /// @todo Create a logical Texture in the "System" scheme for this.
        image_t image;
        if(GL_LoadExtImage(image, "Zeroth", LGM_WHITE_ALPHA))
        {
            // Loaded successfully and converted accordingly.
            // Upload the image to GL.
            pointTex = GL_NewTextureWithParams(
                ( image.pixelSize == 2 ? DGL_LUMINANCE_PLUS_A8 :
                  image.pixelSize == 3 ? DGL_RGB :
                  image.pixelSize == 4 ? DGL_RGBA : DGL_LUMINANCE ),
                image.size.width, image.size.height, image.pixels,
                ( TXCF_MIPMAP | TXCF_NO_COMPRESSION ),
                0, glmode[mipmapping], GL_LINEAR, 0 /*no anisotropy*/, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

            DENG2_ASSERT(pointTex != 0);
        }
        Image_Destroy(&image);
    }
}

void Rend_ParticleLoadExtraTextures()
{
    if(novideo) return;

    Rend_ParticleReleaseExtraTextures();
    if(!App_GameLoaded()) return;

    bool reported = false;
    for(int i = 0; i < MAX_PTC_TEXTURES; ++i)
    {
        if(!loadParticleTexture(i, reported))
        {
            reported = true;
        }
    }
}

void Rend_ParticleReleaseSystemTextures()
{
    if(novideo) return;

    glDeleteTextures(1, (GLuint const *) &pointTex);
    pointTex = 0;
}

void Rend_ParticleReleaseExtraTextures()
{
    if(novideo) return;

    glDeleteTextures(NUM_TEX_NAMES, (GLuint const *) ptctexname);
    de::zap(ptctexname);
}

void Rend_ParticleInitForNewFrame()
{
    // Which set of generators are we using for this frame?
    if(App_World().hasMap())
    {
        /**
         * @todo If rendering multiple maps per frame, this would have to be
         * set from the map rendering function. -jk
         */
        gens = &App_World().map().generators();
    }
    else
    {
        gens = 0;
    }

    if(!useParticles) return;

    // Clear all visibility flags.
    de::zap(visiblePtcGens);
}

void Rend_ParticleMarkInSectorVisible(Sector *sector)
{
    if(!useParticles || !sector) return;

    sector->map().generators().iterateList(sector->indexInMap(), markPtcGenVisible);
}

/**
 * Sorts in descending order.
 */
static int C_DECL comparePOrder(const void* pt1, const void* pt2)
{
    if(((porder_t *) pt1)->distance > ((porder_t *) pt2)->distance) return -1;
    else if(((porder_t *) pt1)->distance < ((porder_t *) pt2)->distance) return 1;
    // Highly unlikely (but possible)...
    return 0;
}

/**
 * Allocate more memory for the particle ordering buffer, if necessary.
 */
static void checkOrderBuffer(size_t max)
{
    size_t currentSize = orderSize;

    if(!orderSize)
    {
        orderSize = MAX_OF(max, 256);
    }
    else
    {
        while(max > orderSize)
            orderSize *= 2;
    }

    if(orderSize > currentSize)
        order = (porder_t *) Z_Realloc(order, sizeof(porder_t) * orderSize, PU_APPSTATIC);
}

static int countParticles(ptcgen_t* gen, void* parameters)
{
    if(isPtcGenVisible(gen))
    {
        size_t* numParts = (size_t*) parameters;
        int p;
        for(p = 0; p < gen->count; ++p)
        {
            if(gen->ptcs[p].stage >= 0)
                (*numParts)++;
        }
    }
    return false; // Continue iteration.
}

static int populateSortBuffer(ptcgen_t *gen, void *parameters)
{
    size_t *sortIndex = (size_t *) parameters;

    if(!isPtcGenVisible(gen))
        return false; // Continue iteration.

    ded_ptcgen_t const *def = gen->def;
    particle_t *pt          = gen->ptcs;
    for(int p = 0; p < gen->count; ++p, pt++)
    {
        if(pt->stage < 0 || !pt->bspLeaf)
            continue;

        // Is the particle's sector visible?
        if(!R_ViewerBspLeafIsVisible(*pt->bspLeaf))
            continue; // No; this particle can't be seen.

        // Don't allow zero distance.
        float dist = de::max(pointDist(pt->origin), 1.f);
        if(def->maxDist != 0 && dist > def->maxDist)
            continue; // Too far.
        if(dist < (float) particleNearLimit)
            continue; // Too near.

        // This particle is visible. Add it to the sort buffer.
        porder_t *slot = &order[(*sortIndex)++];

        slot->ptcGenID = gens->generatorId(gen);
        slot->ptID = p;
        slot->distance = dist;

        // Determine what type of particle this is, as this will affect how
        // we go order our render passes and manipulate the render state.
        int stagetype = gen->stages[pt->stage].type;
        if(stagetype == PTC_POINT)
        {
            hasPoints = true;
        }
        else if(stagetype == PTC_LINE)
        {
            hasLines = true;
        }
        else if(stagetype >= PTC_TEXTURE && stagetype < PTC_TEXTURE + MAX_PTC_TEXTURES)
        {
            if(ptctexname[stagetype - PTC_TEXTURE])
                hasPointTexs[stagetype - PTC_TEXTURE] = true;
            else
                hasPoints = true;
        }
        else if(stagetype >= PTC_MODEL && stagetype < PTC_MODEL + MAX_PTC_MODELS)
        {
            hasModels = true;
        }

        if(gen->flags & PGF_ADD_BLEND)
            hasBlend = true;
        else
            hasNoBlend = true;
    }

    return false; // Continue iteration.
}

/**
 * @return              @c true if there are particles to render.
 */
static int listVisibleParticles()
{
    size_t numVisibleParticles;

    hasPoints = hasModels = hasLines = hasBlend = hasNoBlend = false;
    de::zap(hasPointTexs);

    // First count how many particles are in the visible generators.
    numParts = 0;
    gens->iterate(countParticles, &numParts);
    if(!numParts)
        return false; // No visible generators.

    // Allocate the particle depth sort buffer.
    checkOrderBuffer(numParts);

    // Populate the particle sort buffer and determine what type(s) of
    // particle (model/point/line/etc...) we'll need to draw.
    numVisibleParticles = 0;
    gens->iterate(populateSortBuffer, &numVisibleParticles);
    if(!numVisibleParticles)
        return false; // No visible particles (all too far?).

    // This is the real number of possibly visible particles.
    numParts = numVisibleParticles;

    // Sort the order list back->front. A quicksort is fast enough.
    qsort(order, numParts, sizeof(porder_t), comparePOrder);

    return true;
}

static void setupModelParamsForParticle(rendmodelparams_t* params,
    const particle_t* pt, const ptcstage_t* st, const ded_ptcstage_t* dst,
    const_pvec3f_t origin, float dist, float size, float mark, float alpha)
{
    // Render the particle as a model.
    params->origin[VX] = origin[VX];
    params->origin[VY] = origin[VZ];
    params->origin[VZ] = params->gzt = origin[VY];
    params->distance = dist;

    params->extraScale = size; // Extra scaling factor.
    params->mf = &modefs[dst->model];
    params->alwaysInterpolate = true;

    int frame;
    if(dst->endFrame < 0)
    {
        frame = dst->frame;
        params->inter = 0;
    }
    else
    {
        frame = dst->frame + (dst->endFrame - dst->frame) * mark;
        params->inter = M_CycleIntoRange(mark * (dst->endFrame - dst->frame), 1);
    }

    Rend_ModelSetFrame(params->mf, frame);
    // Set the correct orientation for the particle.
    if(params->mf->testSubFlag(0, MFF_MOVEMENT_YAW))
    {
        params->yaw = R_MovementXYYaw(FIX2FLT(pt->mov[0]), FIX2FLT(pt->mov[1]));
    }
    else
    {
        params->yaw = pt->yaw / 32768.0f * 180;
    }

    if(params->mf->testSubFlag(0, MFF_MOVEMENT_PITCH))
    {
        params->pitch = R_MovementXYZPitch(FIX2FLT(pt->mov[0]), FIX2FLT(pt->mov[1]), FIX2FLT(pt->mov[2]));
    }
    else
    {
        params->pitch = pt->pitch / 32768.0f * 180;
    }

    params->ambientColor[CA] = alpha;

    if((st->flags & PTCF_BRIGHT) || levelFullBright)
    {
        params->ambientColor[CR] = params->ambientColor[CG] = params->ambientColor[CB] = 1;
        params->vLightListIdx = 0;
    }
    else
    {
        Map &map = pt->bspLeaf->map();

        if(useBias && map.hasLightGrid())
        {
            Vector3f tmp = map.lightGrid().evaluate(params->origin);
            V3f_Set(params->ambientColor, tmp.x, tmp.y, tmp.z);
        }
        else
        {
            SectorCluster &cluster = pt->bspLeaf->cluster();
            float lightLevel = cluster.sector().lightLevel();
            Vector3f const &secColor = Rend_SectorLightColor(cluster);

            // Apply distance attenuation.
            lightLevel = Rend_AttenuateLightLevel(params->distance, lightLevel);

            // Add extra light.
            lightLevel = de::clamp(0.f, lightLevel + Rend_ExtraLightDelta(), 1.f);

            Rend_ApplyLightAdaptation(lightLevel);

            // Determine the final ambientColor in affect.
            for(int i = 0; i < 3; ++i)
            {
                params->ambientColor[i] = lightLevel * secColor[i];
            }
        }

        Rend_ApplyTorchLight(params->ambientColor, params->distance);

        collectaffectinglights_params_t lparams; zap(lparams);
        lparams.origin       = Vector3d(params->origin);
        lparams.bspLeaf      = &map.bspLeafAt(Vector2d(origin[VX], origin[VY]));
        lparams.ambientColor = Vector3f(params->ambientColor);

        params->vLightListIdx = R_CollectAffectingLights(&lparams);
    }
}

/**
 * Calculate a unit vector parallel to @a line.
 *
 * @todo No longer needed (Surface has tangent space vectors).
 *
 * @param unitVect  Unit vector is written here.
 */
static void lineUnitVector(Line const &line, pvec2f_t unitVec)
{
    coord_t len = M_ApproxDistance(line.direction().x, line.direction().y);
    if(len)
    {
        unitVec[VX] = line.direction().x / len;
        unitVec[VY] = line.direction().y / len;
    }
    else
    {
        unitVec[VX] = unitVec[VY] = 0;
    }
}

static void renderParticles(int rtype, boolean withBlend)
{
    float leftoff[3], rightoff[3];
    ushort primType = GL_QUADS;
    blendmode_t mode = BM_NORMAL, newMode;
    DGLuint tex = 0;
    size_t i;
    int c;

    DENG_ASSERT_IN_MAIN_THREAD();
    DENG_ASSERT_GL_CONTEXT_ACTIVE();

    viewdata_t const *viewData = R_ViewData(viewPlayer - ddPlayers);
    // viewSideVec points to the left.
    for(c = 0; c < 3; ++c)
    {
        leftoff[c]  = viewData->upVec[c] + viewData->sideVec[c];
        rightoff[c] = viewData->upVec[c] - viewData->sideVec[c];
    }

    // Should we use a texture?
    if(rtype == PTC_POINT ||
       (rtype >= PTC_TEXTURE && rtype < PTC_TEXTURE + MAX_PTC_TEXTURES))
    {
        if(renderTextures)
        {
            if(rtype == PTC_POINT || 0 == ptctexname[rtype - PTC_TEXTURE])
                tex = pointTex;
            else
                tex = ptctexname[rtype - PTC_TEXTURE];
        }
    }

    if(rtype == PTC_MODEL)
    {
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    }
    else if(tex != 0)
    {
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);

        GL_BindTextureUnmanaged(tex, gl::ClampToEdge, gl::ClampToEdge);
        glEnable(GL_TEXTURE_2D);

        glDepthFunc(GL_LEQUAL);
        glBegin(primType = GL_QUADS);
    }
    else
    {
        glBegin(primType = GL_LINES);
    }

    // How many particles can we render?
    if(maxParticles)
        i = numParts - (unsigned) maxParticles;
    else
        i = 0;

    for(; i < numParts; ++i)
    {
        porder_t const *slot = &order[i];
        ptcgen_t const *gen;
        particle_t const *pt;
        ptcstage_t const *st;
        ded_ptcstage_t const *dst, *nextDst;
        float size, color[4], center[3], mark, invMark;
        float dist, maxdist;
        boolean flatOnPlane = false, flatOnWall = false;
        short stageType;

        gen = gens->generator(slot->ptcGenID);
        pt = &gen->ptcs[slot->ptID];

        st = &gen->stages[pt->stage];
        dst = &gen->def->stages[pt->stage];

        stageType = st->type;
        if(stageType >= PTC_TEXTURE && stageType < PTC_TEXTURE + MAX_PTC_TEXTURES &&
           0 == ptctexname[stageType - PTC_TEXTURE])
            stageType = PTC_POINT;

        // Only render one type of particles.
        if((rtype == PTC_MODEL && dst->model < 0) ||
           (rtype != PTC_MODEL && stageType != rtype))
        {
            continue;
        }

        if(rtype >= PTC_TEXTURE && rtype < PTC_TEXTURE + MAX_PTC_TEXTURES &&
           0 == ptctexname[rtype - PTC_TEXTURE])
            continue;

        if(!(gen->flags & PGF_ADD_BLEND) == withBlend)
            continue;

        if(rtype != PTC_MODEL && !withBlend)
        {
            // We may need to change the blending mode.
            newMode =
                (gen->flags & PGF_SUB_BLEND ?    BM_SUBTRACT :
                 gen->flags & PGF_REVSUB_BLEND ? BM_REVERSE_SUBTRACT :
                 gen->flags & PGF_MUL_BLEND ?    BM_MUL :
                 gen->flags & PGF_INVMUL_BLEND ? BM_INVERSE_MUL :
                                                 BM_NORMAL);
            if(newMode != mode)
            {
                glEnd();
                GL_BlendMode(mode = newMode);
                glBegin(primType);
            }
        }

        // Is there a next stage for this particle?
        if(pt->stage >= gen->def->stageCount.num - 1 ||
           !gen->stages[pt->stage + 1].type)
        {   // There is no "next stage". Use the current one.
            nextDst = gen->def->stages + pt->stage;
        }
        else
            nextDst = gen->def->stages + (pt->stage + 1);

        // Where is intermark?
        invMark = pt->tics / (float) dst->tics;
        mark = 1 - invMark;

        // Calculate size and color.
        size = P_GetParticleRadius(dst, slot->ptID) * invMark +
               P_GetParticleRadius(nextDst, slot->ptID) * mark;
        if(!size)
            continue; // Infinitely small.

        for(c = 0; c < 4; ++c)
        {
            color[c] = dst->color[c] * invMark + nextDst->color[c] * mark;
            if(!(st->flags & PTCF_BRIGHT) && c < 3 && !levelFullBright)
            {
                // This is a simplified version of sectorlight (no distance
                // attenuation or range compression).
                if(SectorCluster *cluster = pt->bspLeaf->clusterPtr())
                {
                    color[c] *= cluster->sector().lightLevel();
                }
            }
        }

        maxdist = gen->def->maxDist;
        dist = order[i].distance;

        // Far diffuse?
        if(maxdist)
        {
            if(dist > maxdist * .75f)
                color[3] *= 1 - (dist - maxdist * .75f) / (maxdist * .25f);
        }

        // Near diffuse?
        if(particleDiffuse > 0)
        {
            if(dist < particleDiffuse * size)
                color[3] -= 1 - dist / (particleDiffuse * size);
        }

        // Fully transparent?
        if(color[3] <= 0)
            continue;

        glColor4fv(color);

        boolean nearWall = (pt->contact && !pt->mov[VX] && !pt->mov[VY]);

        boolean nearPlane = false;
        if(SectorCluster *cluster = pt->bspLeaf->clusterPtr())
        {
            if(FLT2FIX(cluster->  visFloor().heightSmoothed()) + 2 * FRACUNIT >= pt->origin[VZ] ||
               FLT2FIX(cluster->visCeiling().heightSmoothed()) - 2 * FRACUNIT <= pt->origin[VZ])
            {
                nearPlane = true;
            }
        }

        if(stageType == PTC_POINT ||
           (stageType >= PTC_TEXTURE && stageType < PTC_TEXTURE + MAX_PTC_TEXTURES))
        {
            if((st->flags & PTCF_PLANE_FLAT) && nearPlane)
                flatOnPlane = true;
            if((st->flags & PTCF_WALL_FLAT) && nearWall)
                flatOnWall = true;
        }

        center[VX] = FIX2FLT(pt->origin[VX]);
        center[VZ] = FIX2FLT(pt->origin[VY]);
        center[VY] = P_GetParticleZ(pt);

        if(!flatOnPlane && !flatOnWall)
        {
            center[VX] += frameTimePos * FIX2FLT(pt->mov[VX]);
            center[VZ] += frameTimePos * FIX2FLT(pt->mov[VY]);
            if(!nearPlane)
                center[VY] += frameTimePos * FIX2FLT(pt->mov[VZ]);
        }

        // Model particles are rendered using the normal model rendering
        // routine.
        if(rtype == PTC_MODEL && dst->model >= 0)
        {
            rendmodelparams_t params;

            memset(&params, 0, sizeof(rendmodelparams_t));

            setupModelParamsForParticle(&params, pt, st, dst, center, dist, size, mark, color[CA]);
            Rend_RenderModel(&params);
            continue;
        }

        // The vertices, please.
        if(tex != 0)
        {
            // Should the particle be flat against a plane?
            if(flatOnPlane)
            {
                glTexCoord2f(0, 0);
                glVertex3f(center[VX] - size, center[VY], center[VZ] - size);

                glTexCoord2f(1, 0);
                glVertex3f(center[VX] + size, center[VY], center[VZ] - size);

                glTexCoord2f(1, 1);
                glVertex3f(center[VX] + size, center[VY], center[VZ] + size);

                glTexCoord2f(0, 1);
                glVertex3f(center[VX] - size, center[VY], center[VZ] + size);
            }
            // Flat against a wall, then?
            else if(flatOnWall)
            {
                vec2d_t origin, projected;

                // There will be a slight approximation on the XY plane since
                // the particles aren't that accurate when it comes to wall
                // collisions.

                // Calculate a new center point (project onto the wall).
                V2d_Set(origin, FIX2FLT(pt->origin[VX]), FIX2FLT(pt->origin[VY]));

                coord_t linePoint[2]     = { pt->contact->fromOrigin().x, pt->contact->fromOrigin().y };
                coord_t lineDirection[2] = { pt->contact->direction().x, pt->contact->direction().y };
                V2d_ProjectOnLine(projected, origin, linePoint, lineDirection);

                // Move away from the wall to avoid the worst Z-fighting.
                double const gap = -1; // 1 map unit.
                double diff[2], dist;
                V2d_Subtract(diff, projected, origin);
                if((dist = V2d_Length(diff)) != 0)
                {
                    projected[VX] += diff[VX] / dist * gap;
                    projected[VY] += diff[VY] / dist * gap;
                }

                DENG_ASSERT(pt->contact != 0);
                float unitvec[2];
                lineUnitVector(*pt->contact, unitvec);

                glTexCoord2f(0, 0);
                glVertex3d(projected[VX] - size * unitvec[VX], center[VY] - size,
                           projected[VY] - size * unitvec[VY]);

                glTexCoord2f(1, 0);
                glVertex3d(projected[VX] - size * unitvec[VX], center[VY] + size,
                           projected[VY] - size * unitvec[VY]);

                glTexCoord2f(1, 1);
                glVertex3d(projected[VX] + size * unitvec[VX], center[VY] + size,
                           projected[VY] + size * unitvec[VY]);

                glTexCoord2f(0, 1);
                glVertex3d(projected[VX] + size * unitvec[VX], center[VY] - size,
                           projected[VY] + size * unitvec[VY]);
            }
            else
            {
                glTexCoord2f(0, 0);
                glVertex3f(center[VX] + size * leftoff[VX],
                           center[VY] + size * leftoff[VY] / 1.2f,
                           center[VZ] + size * leftoff[VZ]);

                glTexCoord2f(1, 0);
                glVertex3f(center[VX] + size * rightoff[VX],
                           center[VY] + size * rightoff[VY] / 1.2f,
                           center[VZ] + size * rightoff[VZ]);

                glTexCoord2f(1, 1);
                glVertex3f(center[VX] - size * leftoff[VX],
                           center[VY] - size * leftoff[VY] / 1.2f,
                           center[VZ] - size * leftoff[VZ]);

                glTexCoord2f(0, 1);
                glVertex3f(center[VX] - size * rightoff[VX],
                           center[VY] - size * rightoff[VY] / 1.2f,
                           center[VZ] - size * rightoff[VZ]);
            }
        }
        else // It's a line.
        {
            glVertex3f(center[VX], center[VY], center[VZ]);
            glVertex3f(center[VX] - FIX2FLT(pt->mov[VX]),
                       center[VY] - FIX2FLT(pt->mov[VZ]),
                       center[VZ] - FIX2FLT(pt->mov[VY]));
        }
    }

    if(rtype != PTC_MODEL)
    {
        glEnd();

        if(tex != 0)
        {
            glEnable(GL_CULL_FACE);
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);

            glDisable(GL_TEXTURE_2D);
        }
    }

    if(!withBlend)
    {
        // We may have rendered subtractive stuff.
        GL_BlendMode(BM_NORMAL);
    }
}

static void renderPass(boolean useBlending)
{
    int i;
    assert(!Sys_GLCheckError());

    // Set blending mode.
    if(useBlending)
        GL_BlendMode(BM_ADD);

    if(hasModels)
        renderParticles(PTC_MODEL, useBlending);

    if(hasLines)
        renderParticles(PTC_LINE, useBlending);

    if(hasPoints)
        renderParticles(PTC_POINT, useBlending);

    for(i = 0; i < NUM_TEX_NAMES; ++i)
    {
        if(hasPointTexs[i])
            renderParticles(PTC_TEXTURE + i, useBlending);
    }

    // Restore blending mode.
    if(useBlending)
        GL_BlendMode(BM_NORMAL);

    assert(!Sys_GLCheckError());
}

void Rend_RenderParticles()
{
    if(!useParticles) return;
    if(!App_World().hasMap()) return;

    // No visible particles at all?
    if(!listVisibleParticles()) return;

    // Render all the visible particles.
    if(hasNoBlend)
    {
        renderPass(false);
    }

    if(hasBlend)
    {
        // A second pass with additive blending.
        // This makes the additive particles 'glow' through all other
        // particles.
        renderPass(true);
    }
}
