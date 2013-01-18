/** @file material.h Logical material.
 *
 * @author Copyright &copy; 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @author Copyright &copy; 2005-2013 Daniel Swanson <danij@dengine.net>
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

#ifndef LIBDENG_RESOURCE_MATERIAL_H
#define LIBDENG_RESOURCE_MATERIAL_H

#ifndef __cplusplus
#  error "resource/material.h requires C++"
#endif

#include "MapElement"
#include "map/p_mapdata.h"
#include "map/p_dmu.h"

typedef enum {
    MEC_UNKNOWN = -1,
    MEC_FIRST = 0,
    MEC_METAL = MEC_FIRST,
    MEC_ROCK,
    MEC_WOOD,
    MEC_CLOTH,
    NUM_MATERIAL_ENV_CLASSES
} material_env_class_t;

#define VALID_MATERIAL_ENV_CLASS(v) ((v) >= MEC_FIRST && (v) < NUM_MATERIAL_ENV_CLASSES)

struct material_variantlist_node_s;

struct material_s; // Opaque type

/**
 * Logical material resource.
 * @todo Rename as Material
 */
class material_t : public de::MapElement
{
public:
    struct ded_material_s* _def;
    struct material_variantlist_node_s* _variants;
    material_env_class_t _envClass;    // Environmental sound class.
    materialid_t        _primaryBind;  // Unique identifier of the MaterialBind associated with this Material or @c NULL if not bound.
    Size2*              _size;         // Logical dimensions in world-space units.
    short               _flags;        // @see materialFlags
    boolean             _inAnimGroup;  // @c true if belongs to some animgroup.
    boolean             _isCustom;
    struct texture_s*   _detailTex;
    float               _detailScale;
    float               _detailStrength;
    struct texture_s*   _shinyTex;
    blendmode_t         _shinyBlendmode;
    float               _shinyMinColor[3];
    float               _shinyStrength;
    struct texture_s*   _shinyMaskTex;
    byte                _prepared;

    material_t();
    ~material_t();

    operator material_s &() {
        return (material_s &) *this;
    }
};

#ifdef __cplusplus

#include <QList>
#include <de/Error>

namespace de {

/**
 * @ingroup resource
 */
class MaterialAnim
{
public:
    /**
     * One frame in the animation.
     */
    class Frame
    {
    public:
        Frame(material_t &mat, ushort _tics, ushort _randomTics)
            : material_(&mat), tics_(_tics), randomTics_(_randomTics)
        {}

        /**
         * Returns the material of the frame.
         */
        material_t &material() const {
            return *material_;
        }

        /**
         * Returns the duration of the frame in (sharp) tics.
         */
        ushort tics() const {
            return tics_;
        }

        /**
         * Returns the random part of the frame duration in (sharp) tics.
         */
        ushort randomTics() const {
            return randomTics_;
        }

    private:
        material_t *material_;
        ushort tics_;
        ushort randomTics_;
    };

    /// All frames in the animation.
    typedef QList<Frame> Frames;

public:
    /// An invalid frame reference was specified. @ingroup errors
    DENG2_ERROR(InvalidFrameError);

public:
    MaterialAnim(int id, int flags);

    /**
     * Progress the animation one frame forward.
     */
    void animate();

    /**
     * Restart the animation over from the first frame.
     */
    void reset();

    /**
     * Returns the animation's unique identifier.
     */
    int id() const;

    /**
     * Returns the animation's @ref animationGroupFlags.
     */
    int flags() const;

    /**
     * Returns the total number of frames in the animation.
     */
    int frameCount() const;

    /**
     * Lookup a frame in the animation by number.
     *
     * @param number  Frame number to lookup.
     * @return  Found animation frame.
     */
    Frame &frame(int number);

    /**
     * Extend the animation by adding a new frame to the end of the sequence.
     *
     * @param mat  Material for the frame.
     * @param tics  Duration of the frame in (sharp) tics.
     * @param randomTics  Random part of the frame duration in (sharp) tics.
     */
    void addFrame(material_t &mat, int tics, int randomTics);

    /**
     * Returns @c true iff @a mat is used by one or more frames in the animation.
     *
     * @param mat  Material to search for.
     */
    bool hasFrameForMaterial(material_t const &mat) const;

    /**
     * Provides access to the frame list for efficient traversal.
     */
    Frames const &allFrames() const;

private:
    /// Unique identifier.
    int id_;

    /// @ref animationGroupFlags.
    int flags_;

    /// Current frame index.
    int index;

    int maxTimer;
    int timer;

    /// All animation frames.
    Frames frames;
};

} // namespace de
extern "C" {
#endif // __cplusplus

struct materialvariant_s;

/**
 * Construct a new material.
 */
material_t* Material_New();

void Material_Delete(material_t* mat);

/**
 * Process a system tick event.
 */
void Material_Ticker(material_t* mat, timespan_t time);

/**
 * Add a new variant to the list of resources for this Material.
 * Material takes ownership of the variant.
 *
 * @param variant  Variant instance to add to the resource list.
 */
struct materialvariant_s* Material_AddVariant(material_t* mat,
    struct materialvariant_s* variant);

/**
 * Destroys all derived MaterialVariants linked with this Material.
 */
void Material_DestroyVariants(material_t* mat);

/**
 * Iterate over all derived MaterialVariants, making a callback for each.
 * Iteration ends once all variants have been visited, or immediately upon
 * a callback returning non-zero.
 *
 * @param callback  Callback to make for each processed variant.
 * @param paramaters  Passed to the callback.
 *
 * @return  @c 0 iff iteration completed wholly.
 */
int Material_IterateVariants(material_t* mat,
    int (*callback)(struct materialvariant_s* variant, void* paramaters),
    void* paramaters);

int Material_VariantCount(material_t const *mat);

/// @return  Definition associated with this.
struct ded_material_s* Material_Definition(const material_t* mat);

/**
 * Change the associated definition.
 * @param def  New definition. Can be @c NULL.
 */
void Material_SetDefinition(material_t* mat, struct ded_material_s* def);

/// Retrieve dimensions in logical world units.
const Size2* Material_Size(const material_t* mat);

/**
 * Change dimensions.
 * @param size  New dimensions in logical world units.
 */
void Material_SetSize(material_t* mat, const Size2Raw* size);

/// @return  Logical width.
int Material_Width(const material_t* mat);

void Material_SetWidth(material_t* mat, int width);

/// @return  Logical height.
int Material_Height(const material_t* mat);

void Material_SetHeight(material_t* mat, int height);

/// @return  @see materialFlags
short Material_Flags(const material_t* mat);

/**
 * Change the public Material Flags.
 * @param flags  @ref materialFlags
 */
void Material_SetFlags(material_t* mat, short flags);

/// @return  @c true if Material is not derived from an original game resource.
boolean Material_IsCustom(const material_t* mat);

/// @return  @c true if Material belongs to one or more anim groups.
boolean Material_IsGroupAnimated(const material_t* mat);

/// @return  @c true if Material should be replaced with Sky.
boolean Material_IsSkyMasked(const material_t* mat);

/// @return  @c true if Material is considered drawable.
boolean Material_IsDrawable(const material_t* mat);

/// @return  @c true if one or more animation stages are defined as "glowing".
boolean Material_HasGlow(material_t* mat);

/// @return  @c true if there is an active translation.
boolean Material_HasTranslation(const material_t* mat);

/// @return  Number of layers.
int Material_LayerCount(const material_t* mat);

/// Change the group animation status.
void Material_SetGroupAnimated(material_t* mat, boolean yes);

/// @return  Prepared state of this material.
byte Material_Prepared(const material_t* mat);

/**
 * Change the prepared status of this material.
 * @param state  @c 0: Not yet prepared.
 *               @c 1: Prepared from original game textures.
 *               @c 2: Prepared from custom or replacement textures.
 */
void Material_SetPrepared(material_t* mat, byte state);

/// @return  Identifier of the primary MaterialBind associated with this (may return @c 0 - no binding).
materialid_t Material_PrimaryBind(const material_t* mat);

/**
 * Change the identifier of the primary binding associated with this.
 * @param bindId  New identifier.
 */
void Material_SetPrimaryBind(material_t* mat, materialid_t bindId);

/// @return  MaterialEnvironmentClass.
material_env_class_t Material_EnvironmentClass(const material_t* mat);

/**
 * Change the associated environment class.
 * \todo If attached to a Map Surface update accordingly!
 * @param envClass  New MaterialEnvironmentClass.
 */
void Material_SetEnvironmentClass(material_t* mat, material_env_class_t envClass);

/// @return  Detail Texture linked to this else @c NULL
struct texture_s* Material_DetailTexture(material_t* mat);

/**
 * Change the Detail Texture linked to this.
 * @param tex  Texture to be linked with.
 */
void Material_SetDetailTexture(material_t* mat, struct texture_s* tex);

/// @return  Detail Texture blending factor for this [0...1].
float Material_DetailStrength(material_t* mat);

/**
 * Change the Detail Texture strength factor for this.
 * @param strength  New strength value (will be clamped to [0...1]).
 */
void Material_SetDetailStrength(material_t* mat, float strength);

/// @return  Detail Texture scale factor for this [0...1].
float Material_DetailScale(material_t* mat);

/**
 * Change the Detail Texture scale factor for this.
 * @param scale  New scale value (will be clamped to [0...1]).
 */
void Material_SetDetailScale(material_t* mat, float scale);

/// @return  Shiny Texture linked to this else @c NULL
struct texture_s* Material_ShinyTexture(material_t* mat);

/**
 * Change the Shiny Texture linked to this.
 * @param tex  Texture to be linked with.
 */
void Material_SetShinyTexture(material_t* mat, struct texture_s* tex);

/// @return  Shiny Texture blendmode for this.
blendmode_t Material_ShinyBlendmode(material_t* mat);

/**
 * Change the Shiny Texture blendmode for this.
 * @param blendmode  New blendmode value.
 */
void Material_SetShinyBlendmode(material_t* mat, blendmode_t blendmode);

/// @return  Shiny Texture strength factor for this.
float Material_ShinyStrength(material_t* mat);

/**
 * Change the Shiny Texture strength factor for this.
 * @param strength  New strength value (will be clamped to [0...1]).
 */
void Material_SetShinyStrength(material_t* mat, float strength);

/// @return  Shiny Texture minColor (RGB triplet) for this.
const float* Material_ShinyMinColor(material_t* mat);

/**
 * Change the Shiny Texture minColor for this.
 * @param colorRGB  New RGB color values (each component will be clamped to [0...1]).
 */
void Material_SetShinyMinColor(material_t* mat, const float colorRGB[3]);

/// @return  ShinyMask Texture linked to this else @c NULL
struct texture_s* Material_ShinyMaskTexture(material_t* mat);

/**
 * Change the ShinyMask Texture linked to this.
 * @param tex  Texture to be linked with.
 */
void Material_SetShinyMaskTexture(material_t* mat, struct texture_s* tex);

/**
 * Get a property value, selected by DMU_* name.
 *
 * @param material  Material instance.
 * @param args  Property arguments.
 * @return  Always @c 0 (can be used as an iterator).
 */
int Material_GetProperty(material_t const *material, setargs_t *args);

/**
 * Update a property value, selected by DMU_* name.
 *
 * @param material  Material instance.
 * @param args  Property arguments.
 * @return  Always @c 0 (can be used as an iterator).
 */
int Material_SetProperty(material_t *material, setargs_t const *args);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* LIBDENG_RESOURCE_MATERIAL_H */