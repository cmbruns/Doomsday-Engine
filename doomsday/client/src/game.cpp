/** @file game.cpp Game mode configuration (metadata, resource files, etc...).
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
 * General Public License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA</small>
 */

#include "de_base.h"
#include "de_console.h"
#include "de_filesys.h"
#include "filesys/manifest.h"
#include <de/Error>
#include <de/Log>
#include <QtAlgorithms>

#include "game.h"

namespace de {

DENG2_PIMPL(Game)
{
    /// Unique identifier of the plugin which registered this game.
    pluginid_t pluginId;

    /// Records for required game files (e.g., doomu.wad).
    Game::Manifests manifests;

    /// Unique identifier string (e.g., "doom1-ultimate").
    ddstring_t identityKey;

    /// Formatted default title suitable for printing (e.g., "The Ultimate DOOM").
    ddstring_t title;

    /// Formatted default author suitable for printing (e.g., "id Software").
    ddstring_t author;

    /// Name of the main config file (e.g., "configs/doom/game.cfg").
    ddstring_t mainConfig;

    /// Name of the file used for control bindings, set automatically at creation time.
    ddstring_t bindingConfig;

    Instance(Public &a, char const *_identityKey, char const *configDir)
        : Base(a), pluginId(0), manifests()
    {
        Str_Set(Str_InitStd(&identityKey), _identityKey);
        //DENG_ASSERT(!Str_IsEmpty(&identityKey));

        Str_InitStd(&title);
        Str_InitStd(&author);

        Str_Appendf(Str_InitStd(&mainConfig), "configs/%s", configDir);
        Str_Strip(&mainConfig);
        F_FixSlashes(&mainConfig, &mainConfig);
        F_AppendMissingSlash(&mainConfig);
        Str_Append(&mainConfig, "game.cfg");

        Str_Appendf(Str_InitStd(&bindingConfig), "configs/%s", configDir);
        Str_Strip(&bindingConfig);
        F_FixSlashes(&bindingConfig, &bindingConfig);
        F_AppendMissingSlash(&bindingConfig);
        Str_Append(&bindingConfig, "player/bindings.cfg");
    }

    ~Instance()
    {
        qDeleteAll(manifests);
        manifests.clear();

        Str_Free(&identityKey);
        Str_Free(&mainConfig);
        Str_Free(&bindingConfig);
        Str_Free(&title);
        Str_Free(&author);
    }
};

Game::Game(char const *identityKey, char const *configDir,
           char const *title, char const *author)
    : game::Game(identityKey),
      d(new Instance(*this, identityKey, configDir))
{
    if(title)  Str_Set(&d->title, title);
    if(author) Str_Set(&d->author, author);
}

Game::~Game()
{}

Game &Game::addManifest(ResourceManifest &manifest)
{
    // Ensure we don't add duplicates.
    Manifests::const_iterator found = d->manifests.find(manifest.resourceClass(), &manifest);
    if(found == d->manifests.end())
    {
        d->manifests.insert(manifest.resourceClass(), &manifest);
    }
    return *this;
}

bool Game::allStartupFilesFound() const
{
    foreach(ResourceManifest *manifest, d->manifests)
    {
        int const flags = manifest->fileFlags();

        if((flags & FF_STARTUP) && !(flags & FF_FOUND))
            return false;
    }
    return true;
}

Game &Game::setPluginId(pluginid_t newId)
{
    d->pluginId = newId;
    return *this;
}

pluginid_t Game::pluginId() const
{
    return d->pluginId;
}

ddstring_t const *Game::identityKey() const
{
    return &d->identityKey;
}

ddstring_t const *Game::mainConfig() const
{
    return &d->mainConfig;
}

ddstring_t const *Game::bindingConfig() const
{
    return &d->bindingConfig;
}

ddstring_t const *Game::title() const
{
    return &d->title;
}

ddstring_t const *Game::author() const
{
    return &d->author;
}

Game::Manifests const &Game::manifests() const
{
    return d->manifests;
}

bool Game::isRequiredFile(File1 &file)
{
    // If this resource is from a container we must use the path of the
    // root file container instead.
    File1 &rootFile = file;
    while(rootFile.isContained())
    { rootFile = rootFile.container(); }

    String absolutePath = rootFile.composePath();
    bool isRequired = false;

    for(Manifests::const_iterator i = d->manifests.find(RC_PACKAGE);
        i != d->manifests.end() && i.key() == RC_PACKAGE; ++i)
    {
        ResourceManifest &manifest = **i;
        if(!(manifest.fileFlags() & FF_STARTUP)) continue;

        if(!manifest.resolvedPath(true/*try locate*/).compare(absolutePath, Qt::CaseInsensitive))
        {
            isRequired = true;
            break;
        }
    }

    return isRequired;
}

Game *Game::fromDef(GameDef const &def)
{
    return new Game(def.identityKey, def.configDir, def.defaultTitle, def.defaultAuthor);
}

void Game::printBanner(Game const &game)
{
    Con_PrintRuler();
    Con_FPrintf(CPF_WHITE | CPF_CENTER, "%s\n", Str_Text(game.title()));
    Con_PrintRuler();
}

void Game::printFiles(Game const &game, int rflags, bool printStatus)
{
    int numPrinted = 0;

    // Group output by resource class.
    Manifests const &manifests = game.manifests();
    for(uint i = 0; i < RESOURCECLASS_COUNT; ++i)
    {
        resourceclassid_t const classId = resourceclassid_t(i);
        for(Manifests::const_iterator i = manifests.find(classId);
            i != manifests.end() && i.key() == classId; ++i)
        {
            ResourceManifest &manifest = **i;
            if(rflags >= 0 && (rflags & manifest.fileFlags()))
            {
                ResourceManifest::consolePrint(manifest, printStatus);
                numPrinted += 1;
            }
        }
    }

    if(numPrinted == 0)
    {
        Con_Printf(" None\n");
    }
}

void Game::print(Game const &game, int flags)
{
    if(game.isNull())
        flags &= ~PGF_BANNER;

#ifdef DENG_DEBUG
    Con_Printf("pluginid:%i\n", int(game.pluginId()));
#endif

    if(flags & PGF_BANNER)
        printBanner(game);

    if(!(flags & PGF_BANNER))
        Con_Printf("Game: %s - ", Str_Text(game.title()));
    else
        Con_Printf("Author: ");
    Con_Printf("%s\n", Str_Text(game.author()));
    Con_Printf("IdentityKey: %s\n", Str_Text(game.identityKey()));

    if(flags & PGF_LIST_STARTUP_RESOURCES)
    {
        Con_Printf("Startup resources:\n");
        printFiles(game, FF_STARTUP, (flags & PGF_STATUS) != 0);
    }

    if(flags & PGF_LIST_OTHER_RESOURCES)
    {
        Con_Printf("Other resources:\n");
        Con_Printf("   ");
        printFiles(game, 0, /*(flags & PGF_STATUS) != 0*/false);
    }

    if(flags & PGF_STATUS)
        Con_Printf("Status: %s\n",
                   &App_CurrentGame() == &game? "Loaded" :
                   game.allStartupFilesFound()? "Complete/Playable" :
                                                "Incomplete/Not playable");
}

NullGame::NullGame()
    : Game("" /*null*/, "doomsday", "null-game", "null-game")
{}

} // namespace de
