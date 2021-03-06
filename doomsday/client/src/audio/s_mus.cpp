/** @file s_mus.cpp Music subsystem. 
 * @ingroup audio
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2007-2013 Daniel Swanson <danij@dengine.net>
 * @authors Copyright © 2006 Jamie Jones <jamie_jones_au@yahoo.com.au>
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

#ifdef __SERVER__
#  error "audio" is not available in a SERVER build
#endif

#include "de_base.h"
#include "de_console.h"
#include "de_system.h"
#include "de_filesys.h"
#include "de_audio.h"
#include "de_misc.h"

#include "audio/sys_audio.h"
#include "audio/m_mus2midi.h"

using namespace de;

D_CMD(PlayMusic);
D_CMD(PauseMusic);
D_CMD(StopMusic);

static void Mus_UpdateSoundFont(void);

static int     musPreference = MUSP_EXT;
static char*   soundFontPath = (char*) "";

static boolean musAvail = false;
static boolean musicPaused = false;
static int     currentSong = -1;

static int getInterfaces(audiointerface_music_generic_t** ifs)
{
    return AudioDriver_FindInterfaces(AUDIO_IMUSIC_OR_ICD, (void**) ifs);
}

void Mus_Register(void)
{
    // Variables:
    C_VAR_INT     ("music-volume",    &musVolume,     0, 0, 255);
    C_VAR_INT     ("music-source",    &musPreference, 0, 0, 2);
    C_VAR_CHARPTR2("music-soundfont", &soundFontPath, 0, 0, 0, Mus_UpdateSoundFont);

    // Commands:
    C_CMD_FLAGS   ("playmusic",  NULL, PlayMusic,  CMDF_NO_DEDICATED);
    C_CMD_FLAGS   ("pausemusic", NULL, PauseMusic, CMDF_NO_DEDICATED);
    C_CMD_FLAGS   ("stopmusic",  "",   StopMusic,  CMDF_NO_DEDICATED);
}

/**
 * Initialize the Mus module.
 *
 * @return  @c true, if no errors occur.
 */
boolean Mus_Init(void)
{
    audiointerface_music_generic_t* iMusic[MAX_AUDIO_INTERFACES];
    int i, count;

    if(musAvail)
        return true; // Already initialized.

    if(isDedicated || CommandLine_Exists("-nomusic"))
    {
        Con_Message("Music disabled.");
        return true;
    }

    VERBOSE( Con_Message("Initializing Music subsystem...") );

    // Let's see which interfaces are available for music playback.
    count = getInterfaces(iMusic);
    currentSong = -1;

    if(!count)
    {
        // No interfaces for Music playback.
        return false;
    }

    // Initialize each interface.
    for(i = 0; i < count; ++i)
    {
        if(!iMusic[i]->Init())
        {
            Con_Message("Warning: Failed to initialize %s for music playback.",
                        Str_Text(AudioDriver_InterfaceName(iMusic[i])));
        }
    }

    // Tell the audio driver about our soundfont config.
    Mus_UpdateSoundFont();

    musAvail = true;
    return true;
}

void Mus_Shutdown(void)
{
    audiointerface_music_generic_t* iMusic[MAX_AUDIO_INTERFACES];
    int i, count;

    if(!musAvail) return;

    musAvail = false;

    // Shutdown interfaces.
    count = getInterfaces(iMusic);
    for(i = 0; i < count; ++i)
    {
        if(iMusic[i]->Shutdown) iMusic[i]->Shutdown();
    }
}

/**
 * Called on each frame by S_StartFrame.
 */
void Mus_StartFrame(void)
{
    audiointerface_music_generic_t* iMusic[MAX_AUDIO_INTERFACES];
    int i, count;

    if(!musAvail) return;

    // Update all interfaces.
    count = getInterfaces(iMusic);
    for(i = 0; i < count; ++i)
    {
        iMusic[i]->Update();
    }
}

/**
 * Set the general music volume. Affects all music played by all interfaces.
 */
void Mus_SetVolume(float vol)
{
    audiointerface_music_generic_t* iMusic[MAX_AUDIO_INTERFACES];
    int i, count;

    if(!musAvail) return;

    // Set volume of all available interfaces.
    count = getInterfaces(iMusic);
    for(i = 0; i < count; ++i)
    {
        iMusic[i]->Set(MUSIP_VOLUME, vol);
    }
}

/**
 * Pauses or resumes the music.
 */
void Mus_Pause(boolean doPause)
{
    audiointerface_music_generic_t* iMusic[MAX_AUDIO_INTERFACES];
    int i, count;

    if(!musAvail) return;

    // Pause all interfaces.
    count = getInterfaces(iMusic);
    for(i = 0; i < count; ++i)
    {
        iMusic[i]->Pause(doPause);
    }
}

void Mus_Stop(void)
{
    audiointerface_music_generic_t* iMusic[MAX_AUDIO_INTERFACES];
    int i, count;

    if(!musAvail) return;

    currentSong = -1;

    // Stop all interfaces.
    count = getInterfaces(iMusic);
    for(i = 0; i < count; ++i)
    {
        iMusic[i]->Stop();
    }
}

/**
 * @return: @c true, if the specified lump contains a MUS song.
 */
boolean Mus_IsMUSLump(lumpnum_t lumpNum)
{
    char buf[4];
    int lumpIdx;
    struct file1_s* file = F_FindFileForLumpNum2(lumpNum, &lumpIdx);
    if(!file) return false;

    F_ReadLumpSection(file, lumpIdx, (uint8_t*)buf, 0, 4);

    // ASCII "MUS" and CTRL-Z (hex 4d 55 53 1a)
    return !strncmp(buf, "MUS\x01a", 4);
}

/**
 * Check for the existence of an "external" music file.
 * Songs can be either in external files or non-MUS lumps.
 *
 * @return  Non-zero if an external file of that name exists.
 */
int Mus_GetExt(ded_music_t *def, ddstring_t *retPath)
{
    LOG_AS("Mus_GetExt");

    if(!musAvail || !AudioDriver_Music_Available()) return false;

    if(def->path && !Str_IsEmpty(Uri_Path(def->path)))
    {
        // All external music files are specified relative to the base path.
        AutoStr *fullPath = AutoStr_NewStd();
        F_PrependBasePath(fullPath, Uri_Path(def->path));
        F_FixSlashes(fullPath, fullPath);

        if(F_Access(Str_Text(fullPath)))
        {
            if(retPath) Str_Set(retPath, Str_Text(fullPath));
            return true;
        }

        LOG_WARNING("Music file \"%s\" not found (id '%s').")
            << *reinterpret_cast<de::Uri *>(def->path) << def->id;
    }

    // Try the resource locator?
    if(def->lumpName[0])
    {
        try
        {
            String foundPath = App_FileSystem().findPath(de::Uri(def->lumpName, RC_MUSIC), RLF_DEFAULT,
                                                         DD_ResourceClassById(RC_MUSIC));
            foundPath = App_BasePath() / foundPath; // Ensure the path is absolute.

            // Does the caller want to know the matched path?
            if(retPath)
            {
                Str_Set(retPath, foundPath.toUtf8().constData());
            }
            return true;
        }
        catch(FS1::NotFoundError const&)
        {} // Ignore this error.
    }
    return false;
}

/**
 * @return  The track number if successful else zero.
 */
int Mus_GetCD(ded_music_t *def)
{
    if(!musAvail || !AudioDriver_CD() || !def)
        return 0;

    if(def->cdTrack)
        return def->cdTrack;

    if(def->path && !stricmp(Str_Text(Uri_Scheme(def->path)), "cd"))
        return atoi(Str_Text(Uri_Path(def->path)));

    return 0;
}

/// @return  Composed music file name.


/**
 * @return 1, if music was started. 0, if attempted to start but failed.
 *         -1, if it was MUS data and @a canPlayMUS says we can't play it.
 */
int Mus_StartLump(lumpnum_t lump, boolean looped, boolean canPlayMUS)
{
    if(!AudioDriver_Music_Available() || lump < 0) return 0;

    if(Mus_IsMUSLump(lump))
    {
        // Lump is in DOOM's MUS format. We must first convert it to MIDI.
        AutoStr* srcFile = 0;
        struct file1_s* file;
        size_t lumpLength;
        int lumpIdx;
        uint8_t* buf;

        if(!canPlayMUS)
            return -1;

        srcFile = AudioDriver_Music_ComposeTempBufferFilename(".mid");

        // Read the lump, convert to MIDI and output to a temp file in the
        // working directory. Use a filename with the .mid extension so that
        // any player which relies on the it for format recognition works as
        // expected.

        lumpLength = F_LumpLength(lump);
        buf = (uint8_t*) malloc(lumpLength);
        if(!buf)
        {
            Con_Message("Warning: Mus_Start: Failed on allocation of %lu bytes for "
                        "temporary MUS to MIDI conversion buffer.", (unsigned long) lumpLength);
            return 0;
        }

        file = F_FindFileForLumpNum2(lump, &lumpIdx);
        F_ReadLumpSection(file, lumpIdx, buf, 0, lumpLength);
        M_Mus2Midi((void*)buf, lumpLength, Str_Text(srcFile));
        free(buf);

        return AudioDriver_Music_PlayNativeFile(Str_Text(srcFile), looped);
    }
    else
    {
        return AudioDriver_Music_PlayLump(lump, looped);
    }
}

/**
 * Start playing a song. The chosen interface depends on what's available
 * and what kind of resources have been associated with the song.
 * Any previously playing song is stopped.
 *
 * @return              Non-zero if the song is successfully played.
 */
int Mus_Start(ded_music_t* def, boolean looped)
{
    int i, order[3], songID;
    ddstring_t path;

    if(!musAvail) return false;

    songID = def - defs.music;

    DEBUG_Message(("Mus_Start: Starting ID:%i looped:%i, currentSong ID:%i\n", songID, looped, currentSong));

    if(songID == currentSong && AudioDriver_Music_IsPlaying())
    {
        // We will not restart the currently playing song.
        return false;
    }

    // Stop the currently playing song.
    Mus_Stop();

    AudioDriver_Music_SwitchBufferFilenames();

    // This is the song we're playing now.
    currentSong = songID;

    // Choose the order in which to try to start the song.
    order[0] = musPreference;

    switch(musPreference)
    {
    case MUSP_CD:
        order[1] = MUSP_EXT;
        order[2] = MUSP_MUS;
        break;

    case MUSP_EXT:
        order[1] = MUSP_MUS;
        order[2] = MUSP_CD;
        break;

    default: // MUSP_MUS
        order[1] = MUSP_EXT;
        order[2] = MUSP_CD;
        break;
    }

    // Try to start the song.
    for(i = 0; i < 3; ++i)
    {
        boolean canPlayMUS = true;

        switch(order[i])
        {
        case MUSP_CD:
            if(Mus_GetCD(def))
            {
                return AudioDriver_Music_PlayCDTrack(Mus_GetCD(def), looped);
            }
            break;

        case MUSP_EXT:
            Str_Init(&path);
            if(Mus_GetExt(def, &path))
            {
                VERBOSE( Con_Message("Attempting to play song '%s' (file \"%s\").",
                                     def->id, F_PrettyPath(Str_Text(&path))) )

                // Its an external file.
                return AudioDriver_Music_PlayFile(Str_Text(&path), looped);
            }

            // Next, try non-MUS lumps.
            canPlayMUS = false;

            // Note: Intentionally falls through to MUSP_MUS.

        case MUSP_MUS:
            if(AudioDriver_Music_Available())
            {
                lumpnum_t lump;
                if(def->lumpName && (lump = F_LumpNumForName(def->lumpName)) >= 0)
                {
                    int result = Mus_StartLump(lump, looped, canPlayMUS);
                    if(result < 0) break;
                    return result;
                }
            }
            break;

        default:
            Con_Error("Mus_Start: Invalid value order[i] = %i.", order[i]);
            break;
        }
    }

    // No song was started.
    return false;
}

static void Mus_UpdateSoundFont(void)
{
    de::NativePath path(soundFontPath);

#ifdef MACOSX
    // On OS X we can try to use the basic DLS soundfont that's part of CoreAudio.
    if(path.isEmpty())
    {
        path = "/System/Library/Components/CoreAudio.component/Contents/Resources/gs_instruments.dls";
    }
#endif

    AudioDriver_Music_Set(AUDIOP_SOUNDFONT_FILENAME,
                          path.expand().toString().toLatin1().constData());
}

/**
 * CCmd: Play a music track.
 */
D_CMD(PlayMusic)
{
    DENG2_UNUSED(src);

    if(!musAvail)
    {
        Con_Printf("The Music module is not available.\n");
        return false;
    }

    switch(argc)
    {
    default:
        Con_Printf("Usage:\n  %s (music-def)\n", argv[0]);
        Con_Printf("  %s lump (lumpname)\n", argv[0]);
        Con_Printf("  %s file (filename)\n", argv[0]);
        Con_Printf("  %s cd (track)\n", argv[0]);
        break;

    case 2: {
        int musIdx = Def_GetMusicNum(argv[1]);
        if(musIdx < 0)
        {
            Con_Printf("Music '%s' not defined.\n", argv[1]);
            return false;
        }

        Mus_Start(&defs.music[musIdx], true);
        break;
      }
    case 3:
        if(!stricmp(argv[1], "lump"))
        {
            lumpnum_t lump = F_LumpNumForName(argv[2]);
            if(lump < 0) return false; // No such lump.

            Mus_Stop();
            return AudioDriver_Music_PlayLump(lump, true);
        }
        else if(!stricmp(argv[1], "file"))
        {
            Mus_Stop();
            return AudioDriver_Music_PlayFile(argv[2], true);
        }
        else
        {   // Perhaps a CD track?
            if(!stricmp(argv[1], "cd"))
            {
                if(!AudioDriver_CD())
                {
                    Con_Printf("No CD audio interface available.\n");
                    return false;
                }

                Mus_Stop();
                return AudioDriver_Music_PlayCDTrack(atoi(argv[2]), true);
            }
        }
        break;
    }

    return true;
}

D_CMD(StopMusic)
{
    DENG2_UNUSED3(src, argc, argv);

    Mus_Stop();
    return true;
}

D_CMD(PauseMusic)
{
    DENG2_UNUSED3(src, argc, argv);

    musicPaused = !musicPaused;
    Mus_Pause(musicPaused);
    return true;
}

