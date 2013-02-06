/** @file materialmanifest.h Description of a logical material resource.
 *
 * @authors Copyright � 2011-2013 Daniel Swanson <danij@dengine.net>
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

#ifndef LIBDENG_RESOURCE_MATERIALMANIFEST_H
#define LIBDENG_RESOURCE_MATERIALMANIFEST_H

#include "Material"
#include "MaterialScheme"
#include "def_data.h"
#include "uri.hh"
#include <de/Error>
#include <de/PathTree>

namespace de {

class Materials;

/**
 * Description of a logical material resource.
 *
 * @ingroup resource
 */
class MaterialManifest : public PathTree::Node
{
public:
    /// Required material instance is missing. @ingroup errors
    DENG2_ERROR(MissingMaterialError);

    enum Flag
    {
        /// The manifest was automatically produced for a game/add-on resource.
        AutoGenerated,

        /// The manifest was not produced for an original game resource.
        Custom
    };
    Q_DECLARE_FLAGS(Flags, Flag)

public:
    MaterialManifest(PathTree::NodeArgs const &args);

    virtual ~MaterialManifest();

    /**
     * Returns the owning scheme of the material manifest.
     */
    MaterialScheme &scheme() const;

    /// Convenience method for returning the name of the owning scheme.
    inline String const &schemeName() const { return scheme().name(); }

    /**
     * Compose a URI of the form "scheme:path" for the material manifest.
     *
     * The scheme component of the URI will contain the symbolic name of
     * the scheme for the material manifest.
     *
     * The path component of the URI will contain the percent-encoded path
     * of the material manifest.
     */
    Uri composeUri(QChar sep = '/') const;

    /**
     * Returns a textual description of the source of the manifest.
     *
     * @return Human-friendly description of the source of the manifest.
     */
    String sourceDescription() const;

    /// @return  Unique identifier associated with the manifest.
    materialid_t id() const;

    void setId(materialid_t newId);

    /**
     * Returns the flags for the manifest.
     */
    Flags flags() const;

    /**
     * Change the manifest's flags.
     *
     * @param flagsToChange  Flags to change the value of.
     * @param set  @c true to set, @c false to clear.
     */
    void setFlags(Flags flagsToChange, bool set = true);

    /// @c true if the manifest was automatically produced for a game/add-on resource.
    inline bool isAutoGenerated() const { return flags().testFlag(AutoGenerated); }

    /// @c true if the manifest was not produced for an original game resource.
    inline bool isCustom() const { return flags().testFlag(Custom); }

    /// @return  @c true if the manifest has an associated material.
    bool hasMaterial() const;

    /**
     * Returns the material associated with the manifest.
     */
    Material &material() const;

    /**
     * Change the material associated with the manifest.
     *
     * @param  material  New material to associate with.
     */
    void setMaterial(Material *material);

    /// Returns a reference to the application's material system.
    static Materials &materials();

private:
    struct Instance;
    Instance *d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MaterialManifest::Flags)

} // namespace de

#endif /* LIBDENG_RESOURCE_MATERIALMANIFEST_H */