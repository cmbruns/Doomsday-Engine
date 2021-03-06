/** @file game.h Game mode configuration (metadata, resource files, etc...).
 *
 * @authors Copyright &copy; 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright &copy; 2005-2013 Daniel Swanson <danij@dengine.net>
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

#ifndef DENG_GAME_H
#define DENG_GAME_H

#include <QMultiMap>

#include "api_plugin.h"
#include <de/ddstring.h>
#include <de/Error>
#include <de/game/Game>

/**
 * @defgroup printGameFlags  Print Game Flags
 * @ingroup flags
 */
///@{
#define PGF_BANNER                 0x1
#define PGF_STATUS                 0x2
#define PGF_LIST_STARTUP_RESOURCES 0x4
#define PGF_LIST_OTHER_RESOURCES   0x8

#define PGF_EVERYTHING             (PGF_BANNER|PGF_STATUS|PGF_LIST_STARTUP_RESOURCES|PGF_LIST_OTHER_RESOURCES)
///@}

struct manifest_s;
struct gamedef_s;

namespace de {

class File1;
class ResourceManifest;

/**
 * Records top-level game configurations registered by the loaded game
 * plugin(s).
 *
 * @ingroup core
 */
class Game : public de::game::Game
{
public:
    typedef QMultiMap<resourceclassid_t, ResourceManifest *> Manifests;

public:
    /**
     * @param identityKey   Unique game mode key/identifier, 16 chars max (e.g., "doom1-ultimate").
     * @param configDir     Name of the config directory.
     * @param title         Default game title.
     * @param author        Default game author.
     */
    Game(char const *identityKey, char const *configDir,
         char const *title = "Unnamed", char const *author = "Unknown");

    virtual ~Game();

    /// @return  Unique plugin identifier attributed to that which registered this.
    pluginid_t pluginId() const;

    /// @return  String containing the identity key.
    ddstring_t const *identityKey() const;

    /// @return  String containing the default title.
    ddstring_t const *title() const;

    /// @return  String containing the default author.
    ddstring_t const *author() const;

    /// @return  String containing the name of the main config file.
    ddstring_t const *mainConfig() const;

    /// @return  String containing the name of the binding config file.
    ddstring_t const *bindingConfig() const;

    /**
     * Change the identfier of the plugin associated with this.
     * @param newId  New identifier.
     */
    Game &setPluginId(pluginid_t newId);

    /**
     * Add a new manifest to the list of manifests.
     *
     * @note Registration order defines load order (among files of the same class).
     *
     * @param manifest  Manifest to add.
     */
    Game &addManifest(ResourceManifest &manifest);

    bool allStartupFilesFound() const;

    /**
     * Provides access to the manifests for efficent traversals.
     */
    Manifests const &manifests() const;

    /**
     * Is @a file required by this game? This decision is made by comparing the
     * absolute path of the specified file to those in the list of located, startup
     * resources for the game. If the file's path matches one of these it is therefore
     * "required" by this game.
     *
     * @param file      File to be tested for required-status. Can be a contained file
     *                  (such as a lump from a Wad file), in which case the path of the
     *                  root (i.e., outermost file) file is used for testing this status.
     *
     * @return  @c true iff @a file is required by this game.
     */
    bool isRequiredFile(File1 &file);

public:
    /**
     * Construct a new Game instance from the specified definition @a def.
     *
     * @note May fail if the definition is incomplete or invalid (@c NULL is returned).
     */
    static Game *fromDef(GameDef const &def);

    /**
     * Print a game mode banner with rulers.
     *
     * @todo This has been moved here so that strings like the game title and author
     *       can be overridden (e.g., via DEHACKED). Make it so!
     */
    static void printBanner(Game const &game);

    /**
     * Print the list of resource files for @a Game.
     *
     * @param game          Game to list the files of.
     * @param rflags        Only consider files whose @ref fileFlags match
     *                      this value. If @c <0 the flags are ignored.
     * @param printStatus   @c true to  include the current availability/load status
     *                      of each file.
     */
    static void printFiles(Game const &game, int rflags, bool printStatus = true);

    /**
     * Print extended information about game @a info.
     *
     * @param game   Game record to be printed.
     * @param flags  @ref printGameFlags
     */
    static void print(Game const &game, int flags);

private:
    DENG2_PRIVATE(d)
};

/**
 * The special "null" Game object.
 * @todo Should employ the Singleton pattern.
 */
class NullGame : public Game
{
public:
    /// General exception for invalid action on a NULL object. @ingroup errors
    DENG2_ERROR(NullObjectError);

public:
    NullGame();

    Game& addManifest(struct manifest_s& /*record*/) {
        throw NullObjectError("NullGame::addResource", "Invalid action on null-object");
    }

    bool isRequiredResource(char const* /*absolutePath*/) {
        return false; // Never.
    }

    bool allStartupFilesFound() const {
        return true; // Always.
    }

    struct manifest_s *const *manifests(resourceclassid_t /*classId*/, int * /*count*/) const {
        return 0;
    }

    static Game *fromDef(GameDef const & /*def*/) {
        throw NullObjectError("NullGame::fromDef", "Not valid for null-object");
    }
};

} // namespace de

#endif /* DENG_GAME_H */
