/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2005-2008 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2005-2008 Daniel Swanson <danij@dengine.net>
 *\author Copyright © 2006 Jamie Jones <jamie_jones_au@yahoo.com.au>
 *\author Copyright © 1993-1996 by id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/**
 * hu_menu.c: Common selection menu, options, episode etc.
 *            Sliders and icons. Kinda widget stuff.
 */

// HEADER FILES ------------------------------------------------------------

#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#if __JDOOM__
#  include "jdoom.h"
#elif __JDOOM64__
#  include "jdoom64.h"
#elif __JHERETIC__
#  include "jheretic.h"
#elif __JHEXEN__
#  include "jhexen.h"
#elif __JSTRIFE__
#  include "jstrife.h"
#endif

#include "m_argv.h"
#include "hu_log.h"
#include "hu_msg.h"
#include "hu_stuff.h"
#include "f_infine.h"
#include "am_map.h"
#include "x_hair.h"
#include "p_player.h"
#include "g_controls.h"
#include "p_saveg.h"
#include "g_common.h"
#include "hu_menu.h"
#include "r_common.h"

// MACROS ------------------------------------------------------------------

#define SAVESTRINGSIZE      24

// TYPES -------------------------------------------------------------------

typedef struct rgba_s {
    float          *r, *g, *b, *a;
} rgba_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

extern void Ed_MakeCursorVisible(void);
void M_InitControlsMenu(void);
void M_ControlGrabDrawer(void);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

void M_NewGame(int option, void* context);
void M_Episode(int option, void* context); // Does nothing in jHEXEN
void M_ChooseClass(int option, void* context); // Does something only in jHEXEN
void M_ChooseSkill(int option, void* context);
void M_LoadGame(int option, void* context);
void M_SaveGame(int option, void* context);
void M_GameFiles(int option, void* context); // Does nothing in jDOOM
void M_EndGame(int option, void* context);
#if !__JDOOM64__
void M_ReadThis(int option, void* context);
void M_ReadThis2(int option, void* context);

# if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
void M_ReadThis3(int option, void* context);
# endif
#endif

void M_QuitDOOM(int option, void* context);

void M_ToggleVar(int option, void* context);

void M_OpenDCP(int option, void* context);
void M_ChangeMessages(int option, void* context);
void M_WeaponAutoSwitch(int option, void* context);
void M_AmmoAutoSwitch(int option, void* context);
void M_HUDInfo(int option, void* context);
void M_HUDScale(int option, void* context);
void M_SfxVol(int option, void* context);
void M_WeaponOrder(int option, void* context);
void M_MusicVol(int option, void* context);
void M_SizeDisplay(int option, void* context);
#if !__JDOOM64__
void M_SizeStatusBar(int option, void* context);
void M_StatusBarAlpha(int option, void* context);
#endif
void M_HUDRed(int option, void* context);
void M_HUDGreen(int option, void* context);
void M_HUDBlue(int option, void* context);
void M_FinishReadThis(int option, void* context);
void M_LoadSelect(int option, void* context);
void M_SaveSelect(int option, void* context);
void M_Xhair(int option, void* context);
void M_XhairSize(int option, void* context);

#if __JDOOM__ || __JDOOM64__
void M_XhairR(int option, void* context);
void M_XhairG(int option, void* context);
void M_XhairB(int option, void* context);
void M_XhairAlpha(int option, void* context);
#endif

void M_DoSave(int slot);
void M_DoLoad(int slot);
static void M_QuickSave(void);
static void M_QuickLoad(void);

#if __JDOOM64__
void M_WeaponRecoil(int option, void* context);
#endif

void M_DrawMainMenu(void);
void M_DrawNewGameMenu(void);
void M_DrawReadThis(void);
void M_DrawSkillMenu(void);
void M_DrawClassMenu(void); // Does something only in jHEXEN
void M_DrawEpisode(void); // Does nothing in jHEXEN
void M_DrawOptions(void);
void M_DrawOptions2(void);
void M_DrawGameplay(void);
void M_DrawHUDMenu(void);
void M_DrawMapMenu(void);
void M_DrawWeaponMenu(void);
void M_DrawLoad(void);
void M_DrawSave(void);
void M_DrawFilesMenu(void);
void M_DrawBackgroundBox(int x, int y, int w, int h,
                         float r, float g, float b, float a,
                         boolean background, int border);

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern editfield_t* ActiveEdit;
extern char* weaponNames[];
extern menu_t ControlsDef;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

#if __JDOOM__ || __JDOOM64__
/// The end message strings will be initialized in Hu_MenuInit().
char* endmsg[NUM_QUITMESSAGES + 1];
#endif

menu_t* currentMenu;

// -1 = no quicksave slot picked!
int quickSaveSlot;

char tempstring[80];

// Old save description before edit.
char saveOldString[SAVESTRINGSIZE+1];

char savegamestrings[10][SAVESTRINGSIZE+1];

// We are going to be entering a savegame string.
int saveStringEnter;
int saveSlot; // Which slot to save in.
int saveCharIndex; // Which char we're editing.

char endstring[160];

static char* yesno[2] = {"NO", "YES"};

#if __JDOOM__ || __JHERETIC__
int epi;
#endif

static char shiftTable[59] = // Contains characters 32 to 90.
{
    /* 32 */ 0, 0, 0, 0, 0, 0, 0, '"',
    /* 40 */ 0, 0, 0, 0, '<', '_', '>', '?', ')', '!',
    /* 50 */ '@', '#', '$', '%', '^', '&', '*', '(', 0, ':',
    /* 60 */ 0, '+', 0, 0, 0, 0, 0, 0, 0, 0,
    /* 70 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 90 */ 0
};

int menu_color = 0;
float skull_angle = 0;

int frame; // Used by any graphic animations that need to be pumped.
int menuTime;

short itemOn; // Menu item skull is on.
short previtemOn; // Menu item skull was last on (for restoring when leaving widget control).
short skullAnimCounter; // Skull animation counter.
short whichSkull; // Which skull to draw.

// Sounds played in the menu.
int menusnds[] = {
#if __JDOOM__ || __JDOOM64__
    SFX_DORCLS,            // close menu
    SFX_SWTCHX,            // open menu
    SFX_SWTCHN,            // cancel
    SFX_PSTOP,             // up/down
    SFX_STNMOV,            // left/right
    SFX_PISTOL,            // accept
    SFX_OOF                // bad sound (eg can't autosave)
#elif __JHERETIC__
    SFX_SWITCH,
    SFX_CHAT,
    SFX_SWITCH,
    SFX_SWITCH,
    SFX_STNMOV,
    SFX_CHAT,
    SFX_CHAT
#elif __JHEXEN__
    SFX_PLATFORM_STOP,
    SFX_DOOR_LIGHT_CLOSE,
    SFX_FIGHTER_HAMMER_HITWALL,
    SFX_PICKUP_KEY,
    SFX_FIGHTER_HAMMER_HITWALL,
    SFX_CHAT,
    SFX_CHAT
#endif
};

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static boolean menuActive;

static float menuAlpha = 0; // Alpha level for the entire menu.
static float menuTargetAlpha = 0; // Target alpha for the entire UI.

#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
static int SkullBaseLump;
#endif

static int cursors = NUMCURSORS;
dpatch_t cursorst[NUMCURSORS];

#if __JHEXEN__
static int MenuPClass;
#endif

static rgba_t widgetcolors[] = { // Ptrs to colors editable with the colour widget
    { &cfg.automapL0[0], &cfg.automapL0[1], &cfg.automapL0[2], NULL },
    { &cfg.automapL1[0], &cfg.automapL1[1], &cfg.automapL1[2], NULL },
    { &cfg.automapL2[0], &cfg.automapL2[1], &cfg.automapL2[2], NULL },
    { &cfg.automapL3[0], &cfg.automapL3[1], &cfg.automapL3[2], NULL },
    { &cfg.automapBack[0], &cfg.automapBack[1], &cfg.automapBack[2], &cfg.automapBack[3] },
    { &cfg.hudColor[0], &cfg.hudColor[1], &cfg.hudColor[2], &cfg.hudColor[3] }
};

static boolean widgetEdit = false; // No active widget by default.
static boolean rgba = false; // Used to swap between rgb / rgba modes for the color widget.

static int editcolorindex = 0; // The index of the widgetcolors array of the item being currently edited.

static float currentcolor[4] = {0, 0, 0, 0}; // Used by the widget as temporay values.

static int menuDarkTicks = 15;
#if !defined( __JHEXEN__ ) && !defined( __JHERETIC__ )
static int quitYet = 0; // Prevents multiple quit responses.
#endif

// Used to fade out the background a little when a widget is active.
static float menu_calpha = 0;

static int quicksave;
static int quickload;

static char notDesignedForMessage[80];

#if __JDOOM__ || __JDOOM64__
static dpatch_t m_doom;
static dpatch_t m_newg;
static dpatch_t m_skill;
static dpatch_t m_episod;
static dpatch_t m_ngame;
static dpatch_t m_option;
static dpatch_t m_loadg;
static dpatch_t m_saveg;
static dpatch_t m_rdthis;
static dpatch_t m_quitg;
static dpatch_t m_optttl;
static dpatch_t dpLSLeft;
static dpatch_t dpLSRight;
static dpatch_t dpLSCntr;
# if __JDOOM__
static dpatch_t credit;
static dpatch_t help;
static dpatch_t help1;
static dpatch_t help2;
# endif
#endif

#if __JHERETIC__ || __JHEXEN__
static dpatch_t m_htic;
static dpatch_t dpFSlot;
#endif

#if __JHERETIC__ || __JHEXEN__
# define READTHISID      3
#elif !__JDOOM64__
# define READTHISID      4
#endif

menuitem_t MainItems[] = {
#if __JDOOM__
    {ITT_SETMENU, 0, "{case}New Game", NULL, MENU_NEWGAME, &m_ngame},
    {ITT_SETMENU, 0, "{case}Options", NULL, MENU_OPTIONS, &m_option},
    {ITT_EFUNC, 0, "{case}Load Game", M_LoadGame, 0, &m_loadg},
    {ITT_EFUNC, 0, "{case}Save Game", M_SaveGame, 0, &m_saveg},
    {ITT_EFUNC, 0, "{case}Read This!", M_ReadThis, 0, &m_rdthis},
    {ITT_EFUNC, 0, "{case}Quit Game", M_QuitDOOM, 0, &m_quitg}
#elif __JDOOM64__
    {ITT_SETMENU, 0, "{case}New Game", NULL, MENU_NEWGAME},
    {ITT_SETMENU, 0, "{case}Options", NULL, MENU_OPTIONS},
    {ITT_EFUNC, 0, "{case}Load Game", M_LoadGame, 0},
    {ITT_EFUNC, 0, "{case}Save Game", M_SaveGame, 0},
    {ITT_EFUNC, 0, "{case}Quit Game", M_QuitDOOM, 0}
#else
    {ITT_SETMENU, 0, "new game", NULL, MENU_NEWGAME},
    {ITT_SETMENU, 0, "options", NULL, MENU_OPTIONS},
    {ITT_SETMENU, 0, "game files", NULL, MENU_FILES},
    {ITT_EFUNC, 0, "info", M_ReadThis, 0},
    {ITT_EFUNC, 0, "quit game", M_QuitDOOM, 0}
#endif
};

menu_t MainDef = {
#if __JHEXEN__
    0,
    110, 50,
    M_DrawMainMenu,
    5, MainItems,
    0, MENU_NONE,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B,
    0, 5
#elif __JHERETIC__
    0,
    110, 64,
    M_DrawMainMenu,
    5, MainItems,
    0, MENU_NONE,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B,
    0, 5
#elif __JSTRIFE__
    0,
    97, 64,
    M_DrawMainMenu,
    6, MainItems,
    0, MENU_NONE,
    huFontA,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B + 1,
    0, 6
#elif __JDOOM64__
    0,
    97, 64,
    M_DrawMainMenu,
    5, MainItems,
    0, MENU_NONE,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B + 1,
    0, 5
#else
    0,
    97, 64,
    M_DrawMainMenu,
    6, MainItems,
    0, MENU_NONE,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B + 1,
    0, 6
#endif
};

menuitem_t NewGameItems[] = {
    {ITT_EFUNC, 0, "S", M_NewGame, 0},
    {ITT_EFUNC, 0, "M", SCEnterMultiplayerMenu, 0}
};

menu_t NewGameDef = {
#if __JHEXEN__
    0,
    110, 50,
    M_DrawNewGameMenu,
    2, NewGameItems,
    0, MENU_MAIN,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B,
    0, 2
#elif __JHERETIC__
    0,
    110, 64,
    M_DrawNewGameMenu,
    2, NewGameItems,
    0, MENU_MAIN,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B,
    0, 2
#elif __JSTRIFE__
    0,
    97, 64,
    M_DrawNewGameMenu,
    2, NewGameItems,
    0, MENU_MAIN,
    huFontA,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B + 1,
    0, 2
#elif __JDOOM64__
    0,
    97, 64,
    M_DrawNewGameMenu,
    2, NewGameItems,
    0, MENU_MAIN,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B + 1,
    0, 2
#else
    0,
    97, 64,
    M_DrawNewGameMenu,
    2, NewGameItems,
    0, MENU_MAIN,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B + 1,
    0, 2
#endif
};

#if __JHEXEN__
static menuitem_t* ClassItems;

menu_t ClassDef = {
    0,
    66, 66,
    M_DrawClassMenu,
    0, NULL,
    0, MENU_NEWGAME,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT_B + 1,
    0, 0
};
#endif

#if __JDOOM__ || __JHERETIC__
static menuitem_t* EpisodeItems;
#endif

#if __JDOOM__ || __JHERETIC__
menu_t EpiDef = {
    0,
    48,
# if __JDOOM__
    63,
# else
    50,
# endif
    M_DrawEpisode,
    0, NULL,
    0, MENU_NEWGAME,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT + 1,
    0, 0
};
#endif


#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
static menuitem_t FilesItems[] = {
    {ITT_EFUNC, 0, "load game", M_LoadGame, 0},
    {ITT_EFUNC, 0, "save game", M_SaveGame, 0}
};

static menu_t FilesMenu = {
    0,
    110, 60,
    M_DrawFilesMenu,
    2, FilesItems,
    0, MENU_MAIN,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT + 1,
    0, 2
};
#endif

static menuitem_t LoadItems[] = {
    {ITT_EFUNC, 0, NULL, M_LoadSelect, 0},
    {ITT_EFUNC, 0, NULL, M_LoadSelect, 1},
    {ITT_EFUNC, 0, NULL, M_LoadSelect, 2},
    {ITT_EFUNC, 0, NULL, M_LoadSelect, 3},
    {ITT_EFUNC, 0, NULL, M_LoadSelect, 4},
    {ITT_EFUNC, 0, NULL, M_LoadSelect, 5},
#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    {ITT_EFUNC, 0, NULL, M_LoadSelect, 6},
    {ITT_EFUNC, 0, NULL, M_LoadSelect, 7}
#endif
};

static menu_t LoadDef = {
    0,
#if __JDOOM__ || __JDOOM64__
    80, 44,
#else
    80, 30,
#endif
    M_DrawLoad,
    NUMSAVESLOTS, LoadItems,
    0, MENU_MAIN,
    huFontA,
    cfg.menuColor2,
    NULL, false,
    LINEHEIGHT_A + 8,
    0, NUMSAVESLOTS
};

static menuitem_t SaveItems[] = {
    {ITT_EFUNC, 0, NULL, M_SaveSelect, 0},
    {ITT_EFUNC, 0, NULL, M_SaveSelect, 1},
    {ITT_EFUNC, 0, NULL, M_SaveSelect, 2},
    {ITT_EFUNC, 0, NULL, M_SaveSelect, 3},
    {ITT_EFUNC, 0, NULL, M_SaveSelect, 4},
    {ITT_EFUNC, 0, NULL, M_SaveSelect, 5},
#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    {ITT_EFUNC, 0, NULL, M_SaveSelect, 6},
    {ITT_EFUNC, 0, NULL, M_SaveSelect, 7}
#endif
};

static menu_t SaveDef = {
    0,
#if __JDOOM__ || __JDOOM64__
    80, 44,
#else
    80, 30,
#endif
    M_DrawSave,
    NUMSAVESLOTS, SaveItems,
    0, MENU_MAIN,
    huFontA,
    cfg.menuColor2,
    NULL, false,
    LINEHEIGHT_A + 8,
    0, NUMSAVESLOTS
};

#if __JSTRIFE__
static menuitem_t SkillItems[] = {
    {ITT_EFUNC, 0, NULL, M_ChooseSkill, SM_BABY},
    {ITT_EFUNC, 0, NULL, M_ChooseSkill, SM_EASY},
    {ITT_EFUNC, 0, NULL, M_ChooseSkill, SM_MEDIUM},
    {ITT_EFUNC, 0, NULL, M_ChooseSkill, SM_HARD},
    {ITT_EFUNC, 0, NULL, M_ChooseSkill, SM_NIGHTMARE}
};

static menu_t SkillDef = {
    0,
    120, 44,
    M_DrawSkillMenu,
    5, SkillItems,
    2, MENU_NEWGAME,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT,
    0, 5
};

#elif __JHEXEN__
static menuitem_t SkillItems[] = {
    {ITT_EFUNC, 0, NULL, M_ChooseSkill, SM_BABY},
    {ITT_EFUNC, 0, NULL, M_ChooseSkill, SM_EASY},
    {ITT_EFUNC, 0, NULL, M_ChooseSkill, SM_MEDIUM},
    {ITT_EFUNC, 0, NULL, M_ChooseSkill, SM_HARD},
    {ITT_EFUNC, 0, NULL, M_ChooseSkill, SM_NIGHTMARE}
};

static menu_t SkillDef = {
    0,
    120, 44,
    M_DrawSkillMenu,
    5, SkillItems,
    2, MENU_CLASS,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT,
    0, 5
};
#elif __JHERETIC__
static menuitem_t SkillItems[] = {
    {ITT_EFUNC, 0, "W", M_ChooseSkill, SM_BABY},
    {ITT_EFUNC, 0, "Y", M_ChooseSkill, SM_EASY},
    {ITT_EFUNC, 0, "B", M_ChooseSkill, SM_MEDIUM},
    {ITT_EFUNC, 0, "S", M_ChooseSkill, SM_HARD},
    {ITT_EFUNC, 0, "P", M_ChooseSkill, SM_NIGHTMARE}
};

static menu_t SkillDef = {
    0,
    38, 30,
    M_DrawSkillMenu,
    5, SkillItems,
    2, MENU_EPISODE,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT,
    0, 5
};
#elif __JDOOM64__
static menuitem_t SkillItems[] = {
    {ITT_EFUNC, 0, "I", M_ChooseSkill, 0, &skillModeNames[0]},
    {ITT_EFUNC, 0, "H", M_ChooseSkill, 1, &skillModeNames[1]},
    {ITT_EFUNC, 0, "H", M_ChooseSkill, 2, &skillModeNames[2]},
    {ITT_EFUNC, 0, "U", M_ChooseSkill, 3, &skillModeNames[3]},
};
static menu_t SkillDef = {
    0,
    48, 63,
    M_DrawSkillMenu,
    4, SkillItems,
    2, MENU_NEWGAME,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT,
    0, 4
};
#else
static menuitem_t SkillItems[] = {
    // Text defs TXT_SKILL1..5.
    {ITT_EFUNC, 0, "I", M_ChooseSkill, 0, &skillModeNames[0]},
    {ITT_EFUNC, 0, "H", M_ChooseSkill, 1, &skillModeNames[1]},
    {ITT_EFUNC, 0, "H", M_ChooseSkill, 2, &skillModeNames[2]},
    {ITT_EFUNC, 0, "U", M_ChooseSkill, 3, &skillModeNames[3]},
    {ITT_EFUNC, MIF_NOTALTTXT, "N", M_ChooseSkill, 4, &skillModeNames[4]}
};

static menu_t SkillDef = {
    0,
    48, 63,
    M_DrawSkillMenu,
    5, SkillItems,
    2, MENU_EPISODE,
    huFontB,
    cfg.menuColor,
    NULL, false,
    LINEHEIGHT,
    0, 5
};
#endif

static menuitem_t OptionsItems[] = {
    {ITT_EFUNC, 0, "end game", M_EndGame, 0},
    {ITT_EFUNC, 0, "control panel", M_OpenDCP, 0},
    {ITT_SETMENU, 0, "controls...", NULL, MENU_CONTROLS},
    {ITT_SETMENU, 0, "gameplay...", NULL, MENU_GAMEPLAY},
    {ITT_SETMENU, 0, "hud...", NULL, MENU_HUD},
    {ITT_SETMENU, 0, "automap...", NULL, MENU_MAP},
    {ITT_SETMENU, 0, "weapons...", NULL, MENU_WEAPONSETUP},
    {ITT_SETMENU, 0, "sound...", NULL, MENU_OPTIONS2},
    {ITT_EFUNC, 0, "mouse...", M_OpenDCP, 2},
    {ITT_EFUNC, 0, "joystick...", M_OpenDCP, 2}
};

static menu_t OptionsDef = {
    0,
    94, 63,
    M_DrawOptions,
    10, OptionsItems,
    0, MENU_MAIN,
    huFontA,
    cfg.menuColor2,
    NULL, false,
    LINEHEIGHT_A,
    0, 10
};

static menuitem_t Options2Items[] = {
    {ITT_LRFUNC, 0, "SFX VOLUME :", M_SfxVol, 0},
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
#endif
    {ITT_LRFUNC, 0, "MUSIC VOLUME :", M_MusicVol, 0},
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
#endif
    {ITT_EFUNC, 0, "OPEN AUDIO PANEL", M_OpenDCP, 1},
};

static menu_t Options2Def = {
    0,
#if __JHEXEN__ || __JSTRIFE__
    70, 25,
#elif __JHERETIC__
    70, 30,
#elif __JDOOM__ || __JDOOM64__
    70, 40,
#endif
    M_DrawOptions2,
#if __JDOOM__ || __JDOOM64__
    3, Options2Items,
#else
    7, Options2Items,
#endif
    0, MENU_OPTIONS,
    huFontA,
    cfg.menuColor2,
    NULL, false,
    LINEHEIGHT_A,
#if __JDOOM__ || __JDOOM64__
    0, 3
#else
    0, 7
#endif
};

#if !__JDOOM64__
menuitem_t ReadItems1[] = {
    {ITT_EFUNC, 0, "", M_ReadThis2, 0}
};

menu_t ReadDef1 = {
    MNF_NOSCALE,
    280, 185,
    M_DrawReadThis,
    1, ReadItems1,
    0, MENU_MAIN,
    huFontB,
    cfg.menuColor,
    "HELP1",
#if __JDOOM__
    false,
#else
    true,
#endif
    LINEHEIGHT,
    0, 1
};

menuitem_t ReadItems2[] = {
# if __JDOOM__
    {ITT_EFUNC, 0, "", M_FinishReadThis, 0}
# else
    {ITT_EFUNC, 0, "", M_ReadThis3, 0} // heretic and hexen have 3 readthis screens.
# endif
};

menu_t ReadDef2 = {
    MNF_NOSCALE,
    330, 175,
    M_DrawReadThis,
    1, ReadItems2,
    0, MENU_MAIN,
    huFontB,
    cfg.menuColor,
    "HELP2",
#if __JDOOM__
    false,
#else
    true,
#endif
    LINEHEIGHT,
    0, 1
};

# if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
menuitem_t ReadItems3[] = {
    {ITT_EFUNC, 0, "", M_FinishReadThis, 0}
};

menu_t ReadDef3 = {
    MNF_NOSCALE,
    330, 175,
    M_DrawReadThis,
    1, ReadItems3,
    0, MENU_MAIN,
    huFontB,
    cfg.menuColor,
    "CREDIT", true,
    LINEHEIGHT,
    0, 1
};
# endif
#endif

static menuitem_t HUDItems[] = {
#if __JDOOM64__
    {ITT_EFUNC, 0, "show ammo :", M_ToggleVar, 0, NULL, "hud-ammo" },
    {ITT_EFUNC, 0, "show armor :", M_ToggleVar, 0, NULL, "hud-armor" },
    {ITT_EFUNC, 0, "show power keys :", M_ToggleVar, 0, NULL, "hud-power" },
    {ITT_EFUNC, 0, "show health :", M_ToggleVar, 0, NULL,"hud-health" },
    {ITT_EFUNC, 0, "show keys :", M_ToggleVar, 0, NULL, "hud-keys" },
    {ITT_LRFUNC, 0, "scale", M_HUDScale, 0},
    {ITT_EFUNC, 0, "   HUD color", SCColorWidget, 5},
#elif __JDOOM__
    {ITT_EFUNC, 0, "show ammo :", M_ToggleVar, 0, NULL, "hud-ammo"},
    {ITT_EFUNC, 0, "show armor :", M_ToggleVar, 0, NULL, "hud-armor"},
    {ITT_EFUNC, 0, "show face :", M_ToggleVar, 0, NULL, "hud-face"},
    {ITT_EFUNC, 0, "show health :", M_ToggleVar, 0, NULL, "hud-health"},
    {ITT_EFUNC, 0, "show keys :", M_ToggleVar, 0, NULL, "hud-keys"},
    {ITT_EFUNC, 0, "single key display :", M_ToggleVar, 0, NULL, "hud-keys-combine"},
    {ITT_LRFUNC, 0, "scale", M_HUDScale, 0},
    {ITT_EFUNC, 0, "   HUD color", SCColorWidget, 5},
#endif
    {ITT_EFUNC, 0, "MESSAGES :", M_ChangeMessages, 0},
    {ITT_LRFUNC, 0, "CROSSHAIR :", M_Xhair, 0},
    {ITT_LRFUNC, 0, "CROSSHAIR SIZE :", M_XhairSize, 0},
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
#endif
    {ITT_LRFUNC, 0, "SCREEN SIZE", M_SizeDisplay, 0},
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
#endif
#if !__JDOOM64__
    {ITT_LRFUNC, 0, "STATUS BAR SIZE", M_SizeStatusBar, 0},
# if !__JDOOM__
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
# endif
    {ITT_LRFUNC, 0, "STATUS BAR ALPHA :", M_StatusBarAlpha, 0},
# if !__JDOOM__
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
# endif
#endif
#if __JHEXEN__ || __JSTRIFE__
    {ITT_INERT, 0, "FULLSCREEN HUD",    NULL, 0},
    {ITT_INERT, 0, "FULLSCREEN HUD",    NULL, 0},
    {ITT_EFUNC, 0, "SHOW MANA :", M_ToggleVar, 0, NULL, "hud-mana" },
    {ITT_EFUNC, 0, "SHOW HEALTH :", M_ToggleVar, 0, NULL, "hud-health" },
    {ITT_EFUNC, 0, "SHOW ARTIFACT :", M_ToggleVar, 0, NULL, "hud-artifact" },
    {ITT_EFUNC, 0, "   HUD COLOUR", SCColorWidget, 5},
    {ITT_LRFUNC, 0, "SCALE", M_HUDScale, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
#elif __JHERETIC__
    {ITT_INERT, 0, "FULLSCREEN HUD",    NULL, 0},
    {ITT_INERT, 0, "FULLSCREEN HUD",    NULL, 0},
    {ITT_EFUNC, 0, "SHOW AMMO :", M_ToggleVar, 0, NULL, "hud-ammo" },
    {ITT_EFUNC, 0, "SHOW ARMOR :", M_ToggleVar, 0, NULL, "hud-armor" },
    {ITT_EFUNC, 0, "SHOW ARTIFACT :", M_ToggleVar, 0, NULL, "hud-artifact" },
    {ITT_EFUNC, 0, "SHOW HEALTH :", M_ToggleVar, 0, NULL, "hud-health" },
    {ITT_EFUNC, 0, "SHOW KEYS :", M_ToggleVar, 0, NULL, "hud-keys" },
    {ITT_EFUNC, 0, "   HUD COLOUR", SCColorWidget, 5},
    {ITT_LRFUNC, 0, "SCALE", M_HUDScale, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0}
#endif
};

static menu_t HUDDef = {
    0,
#if __JDOOM__ || __JDOOM64__
    70, 40,
#else
    64, 30,
#endif
    M_DrawHUDMenu,
#if __JHEXEN__ || __JSTRIFE__
    23, HUDItems,
#elif __JHERETIC__
    25, HUDItems,
#elif __JDOOM64__
    11, HUDItems,
#elif __JDOOM__
    14, HUDItems,
#endif
    0, MENU_OPTIONS,
    huFontA,
    cfg.menuColor2,
    NULL, false,
    LINEHEIGHT_A,
#if __JHEXEN__ || __JSTRIFE__
    0, 15        // 21
#elif __JHERETIC__
    0, 15        // 23
#elif __JDOOM64__
    0, 11
#elif __JDOOM__
    0, 14
#endif
};

static menuitem_t WeaponItems[] = {
    {ITT_EMPTY,  0, "Use left/right to move", NULL, 0 },
    {ITT_EMPTY,  0, "item up/down the list." , NULL, 0 },
    {ITT_EMPTY,  0, NULL, NULL, 0},
    {ITT_EMPTY,  0, "PRIORITY ORDER", NULL, 0},
    {ITT_LRFUNC, 0, "1 :", M_WeaponOrder, 0 << NUM_WEAPON_TYPES },
    {ITT_LRFUNC, 0, "2 :", M_WeaponOrder, 1 << NUM_WEAPON_TYPES },
    {ITT_LRFUNC, 0, "3 :", M_WeaponOrder, 2 << NUM_WEAPON_TYPES },
    {ITT_LRFUNC, 0, "4 :", M_WeaponOrder, 3 << NUM_WEAPON_TYPES },
#if !__JHEXEN__
    {ITT_LRFUNC, 0, "5 :", M_WeaponOrder, 4 << NUM_WEAPON_TYPES },
    {ITT_LRFUNC, 0, "6 :", M_WeaponOrder, 5 << NUM_WEAPON_TYPES },
    {ITT_LRFUNC, 0, "7 :", M_WeaponOrder, 6 << NUM_WEAPON_TYPES },
    {ITT_LRFUNC, 0, "8 :", M_WeaponOrder, 7 << NUM_WEAPON_TYPES },
#endif
#if __JDOOM__ || __JDOOM64__
    {ITT_LRFUNC, 0, "9 :", M_WeaponOrder, 8 << NUM_WEAPON_TYPES },
#endif
#if __JDOOM64__
    {ITT_LRFUNC, 0, "10 :", M_WeaponOrder, 9 << NUM_WEAPON_TYPES },
#endif
    {ITT_EFUNC,  0, "Use with Next/Previous :", M_ToggleVar, 0, NULL, "player-weapon-nextmode"},
    {ITT_EMPTY,  0, NULL, NULL, 0},
    {ITT_EMPTY,  0, "AUTOSWITCH", NULL, 0},
    {ITT_LRFUNC, 0, "PICKUP WEAPON :", M_WeaponAutoSwitch, 0},
    {ITT_EFUNC,  0, "   IF NOT FIRING :", M_ToggleVar, 0, NULL, "player-autoswitch-notfiring"},
    {ITT_LRFUNC, 0, "PICKUP AMMO :", M_AmmoAutoSwitch, 0},
#if __JDOOM__ || __JDOOM64__
    {ITT_EFUNC,  0, "PICKUP BERSERK :", M_ToggleVar, 0, NULL, "player-autoswitch-berserk"}
#endif
};

static menu_t WeaponDef = {
    MNF_NOHOTKEYS,
#if __JDOOM__ || __JDOOM64__
    68, 34,
#else
    50, 20,
#endif
    M_DrawWeaponMenu,
#if __JDOOM64__
    21, WeaponItems,
#elif __JDOOM__
    20, WeaponItems,
#elif __JHERETIC__
    18, WeaponItems,
#elif __JHEXEN__
    14, WeaponItems,
#endif
    0, MENU_OPTIONS,
    huFontA,
    cfg.menuColor2,
    NULL, false,
    LINEHEIGHT_A,
#if __JDOOM64__
    0, 21
#elif __JDOOM__
    0, 20
#elif __JHERETIC__
    0, 18
#elif __JHEXEN__
    0, 14
#endif
};

static menuitem_t GameplayItems[] = {
    {ITT_EFUNC, 0, "ALWAYS RUN :", M_ToggleVar, 0, NULL, "ctl-run"},
    {ITT_EFUNC, 0, "USE LOOKSPRING :", M_ToggleVar, 0, NULL, "ctl-look-spring"},
    {ITT_EFUNC, 0, "USE AUTOAIM :", M_ToggleVar, 0, NULL, "ctl-aim-noauto"},
#if __JDOOM__ || __JHERETIC__ || __JDOOM64__ || __JSTRIFE__
    {ITT_EFUNC, 0, "ALLOW JUMPING :", M_ToggleVar, 0, NULL, "player-jump"},
#endif

#if __JDOOM64__
    { ITT_EFUNC, 0, "WEAPON RECOIL : ", M_WeaponRecoil, 0 },
#endif

#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, "COMPATIBILITY", NULL, 0 },
# if __JDOOM__ || __JDOOM64__
    {ITT_EFUNC, 0, "ANY BOSS TRIGGER 666 :", M_ToggleVar, 0, NULL,
        "game-anybossdeath666"},
#  if !__JDOOM64__
    {ITT_EFUNC, 0, "AV RESURRECTS GHOSTS :", M_ToggleVar, 0, NULL,
        "game-raiseghosts"},
#  endif
    {ITT_EFUNC, 0, "PE LIMITED TO 20 LOST SOULS :", M_ToggleVar, 0, NULL,
        "game-maxskulls"},
    {ITT_EFUNC, 0, "LS CAN GET STUCK INSIDE WALLS :", M_ToggleVar, 0, NULL,
        "game-skullsinwalls"},
# endif
# if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    {ITT_EFUNC, 0, "MONSTERS CAN GET STUCK IN DOORS :", M_ToggleVar, 0, NULL,
        "game-monsters-stuckindoors"},
    {ITT_EFUNC, 0, "SOME OBJECTS NEVER HANG OVER LEDGES :", M_ToggleVar, 0, NULL,
        "game-objects-neverhangoverledges"},
    {ITT_EFUNC, 0, "OBJECTS FALL UNDER OWN WEIGHT :", M_ToggleVar, 0, NULL,
        "game-objects-falloff"},
    {ITT_EFUNC, 0, "CORPSES SLIDE DOWN STAIRS :", M_ToggleVar, 0, NULL,
        "game-corpse-sliding"},
    {ITT_EFUNC, 0, "USE EXACTLY DOOM'S CLIPPING CODE :", M_ToggleVar, 0, NULL,
        "game-objects-clipping"},
    {ITT_EFUNC, 0, "  ^IFNOT NORTHONLY WALLRUNNING :", M_ToggleVar, 0, NULL,
        "game-player-wallrun-northonly"},
# endif
# if __JDOOM__ || __JDOOM64__
    {ITT_EFUNC, 0, "ZOMBIE PLAYERS CAN EXIT MAPS :", M_ToggleVar, 0, NULL,
        "game-zombiescanexit"},
    {ITT_EFUNC, 0, "FIX OUCH FACE :", M_ToggleVar, 0, NULL, "hud-face-ouchfix"},
# endif
#endif
};

#if __JHEXEN__
static menu_t GameplayDef = {
    0,
    64, 25,
    M_DrawGameplay,
    3, GameplayItems,
    0, MENU_OPTIONS,
    huFontA,
    cfg.menuColor2,
    NULL, false,
    LINEHEIGHT_A,
    0, 3
};
#else
static menu_t GameplayDef = {
    0,
#if __JHERETIC__
    30, 30,
#else
    30, 40,
#endif
    M_DrawGameplay,
#if __JDOOM64__
    17, GameplayItems,
#elif __JDOOM__
    18, GameplayItems,
#else
    12, GameplayItems,
#endif
    0, MENU_OPTIONS,
    huFontA,
    cfg.menuColor2,
    NULL, false,
    LINEHEIGHT_A,
#if __JDOOM64__
    0, 17
#elif __JDOOM__
    0, 18
#else
    0, 12
#endif
};
#endif

menu_t* menulist[] = {
    &MainDef,
    &NewGameDef,
#if __JHEXEN__
    &ClassDef,
#endif
#if __JDOOM__ || __JHERETIC__
    &EpiDef,
#endif
    &SkillDef,
    &OptionsDef,
    &Options2Def,
    &GameplayDef,
    &HUDDef,
    &MapDef,
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    &FilesMenu,
#endif
    &LoadDef,
    &SaveDef,
    &MultiplayerMenu,
    &GameSetupMenu,
    &PlayerSetupMenu,
    &WeaponDef,
    &ControlsDef,
    NULL
};

static menuitem_t ColorWidgetItems[] = {
    {ITT_LRFUNC, 0, "red :    ", M_WGCurrentColor, 0, NULL, &currentcolor[0] },
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
#endif
    {ITT_LRFUNC, 0, "green :", M_WGCurrentColor, 0, NULL, &currentcolor[1] },
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
#endif
    {ITT_LRFUNC, 0, "blue :  ", M_WGCurrentColor, 0, NULL, &currentcolor[2] },
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    {ITT_EMPTY, 0, NULL, NULL, 0},
    {ITT_EMPTY, 0, NULL, NULL, 0},
#endif
    {ITT_LRFUNC, 0, "alpha :", M_WGCurrentColor, 0, NULL, &currentcolor[3] },
};

static menu_t ColorWidgetMnu = {
    MNF_NOHOTKEYS,
    98, 60,
    NULL,
#if __JDOOM__ || __JDOOM64__
    4, ColorWidgetItems,
#else
    10, ColorWidgetItems,
#endif
    0, MENU_OPTIONS,
    huFontA,
    cfg.menuColor2,
    NULL, false,
    LINEHEIGHT_A,
#if __JDOOM__ || __JDOOM64__
    0, 4
#else
    0, 10
#endif
};

// Cvars for the menu
cvar_t menuCVars[] =
{
    {"menu-scale", 0, CVT_FLOAT, &cfg.menuScale, .1f, 1},
    {"menu-flash-r", 0, CVT_FLOAT, &cfg.flashColor[0], 0, 1},
    {"menu-flash-g", 0, CVT_FLOAT, &cfg.flashColor[1], 0, 1},
    {"menu-flash-b", 0, CVT_FLOAT, &cfg.flashColor[2], 0, 1},
    {"menu-flash-speed", 0, CVT_INT, &cfg.flashSpeed, 0, 50},
    {"menu-turningskull", 0, CVT_BYTE, &cfg.turningSkull, 0, 1},
    {"menu-effect", 0, CVT_INT, &cfg.menuEffects, 0, 2},
    {"menu-color-r", 0, CVT_FLOAT, &cfg.menuColor[0], 0, 1},
    {"menu-color-g", 0, CVT_FLOAT, &cfg.menuColor[1], 0, 1},
    {"menu-color-b", 0, CVT_FLOAT, &cfg.menuColor[2], 0, 1},
    {"menu-colorb-r", 0, CVT_FLOAT, &cfg.menuColor2[0], 0, 1},
    {"menu-colorb-g", 0, CVT_FLOAT, &cfg.menuColor2[1], 0, 1},
    {"menu-colorb-b", 0, CVT_FLOAT, &cfg.menuColor2[2], 0, 1},
    {"menu-glitter", 0, CVT_FLOAT, &cfg.menuGlitter, 0, 1},
    {"menu-fog", 0, CVT_INT, &cfg.hudFog, 0, 4},
    {"menu-shadow", 0, CVT_FLOAT, &cfg.menuShadow, 0, 1},
    {"menu-patch-replacement", 0, CVT_BYTE, &cfg.usePatchReplacement, 0, 2},
    {"menu-slam", 0, CVT_BYTE, &cfg.menuSlam, 0, 1},
    {"menu-quick-ask", 0, CVT_BYTE, &cfg.askQuickSaveLoad, 0, 1},
#if __JDOOM__ || __JDOOM64__
    {"menu-quitsound", 0, CVT_INT, &cfg.menuQuitSound, 0, 1},
#endif
    {NULL}
};

// Console commands for the menu
ccmd_t menuCCmds[] = {
    {"menu",        "", CCmdMenuAction},
    {"menuup",      "", CCmdMenuAction},
    {"menudown",    "", CCmdMenuAction},
    {"menuleft",    "", CCmdMenuAction},
    {"menuright",   "", CCmdMenuAction},
    {"menuselect",  "", CCmdMenuAction},
    {"menudelete",  "", CCmdMenuAction},
    {"menuback",    "", CCmdMenuAction},
    {"helpscreen",  "", CCmdMenuAction},
    {"savegame",    "", CCmdMenuAction},
    {"loadgame",    "", CCmdMenuAction},
    {"soundmenu",   "", CCmdMenuAction},
    {"quicksave",   "", CCmdMenuAction},
    {"endgame",     "", CCmdMenuAction},
    {"togglemsgs",  "", CCmdMenuAction},
    {"quickload",   "", CCmdMenuAction},
    {"quit",        "", CCmdMenuAction},
    {"togglegamma", "", CCmdMenuAction},
    {NULL}
};

// Code -------------------------------------------------------------------

/**
 * Called during the PreInit of each game during start up
 * Register Cvars and CCmds for the opperation/look of the menu.
 */
void Hu_MenuRegister(void)
{
    int             i;

    for(i = 0; menuCVars[i].name; ++i)
        Con_AddVariable(menuCVars + i);
    for(i = 0; menuCCmds[i].name; ++i)
        Con_AddCommand(menuCCmds + i);
}

/**
 * Load any resources the menu needs.
 */
void M_LoadData(void)
{
    int             i;
    char            buffer[9];

    // Load the cursor patches
    for(i = 0; i < cursors; ++i)
    {
        sprintf(buffer, CURSORPREF, i+1);
        R_CachePatch(&cursorst[i], buffer);
    }

#if __JDOOM__ || __JDOOM64__
    R_CachePatch(&m_doom, "M_DOOM");
    R_CachePatch(&m_newg, "M_NEWG");
    R_CachePatch(&m_skill, "M_SKILL");
    R_CachePatch(&m_episod, "M_EPISOD");
    R_CachePatch(&m_ngame, "M_NGAME");
    R_CachePatch(&m_option, "M_OPTION");
    R_CachePatch(&m_loadg, "M_LOADG");
    R_CachePatch(&m_saveg, "M_SAVEG");
    R_CachePatch(&m_rdthis, "M_RDTHIS");
    R_CachePatch(&m_quitg, "M_QUITG");
    R_CachePatch(&m_optttl, "M_OPTTTL");
    R_CachePatch(&dpLSLeft, "M_LSLEFT");
    R_CachePatch(&dpLSRight, "M_LSRGHT");
    R_CachePatch(&dpLSCntr, "M_LSCNTR");
# if __JDOOM__
    if(gameMode == retail || gameMode == commercial)
        R_CachePatch(&credit, "CREDIT");
    if(gameMode == commercial)
        R_CachePatch(&help, "HELP");
    if(gameMode == shareware || gameMode == registered || gameMode == retail)
        R_CachePatch(&help1, "HELP1");
    if(gameMode == shareware || gameMode == registered)
        R_CachePatch(&help2, "HELP2");
# endif
#endif

#if __JHERETIC__ || __JHEXEN__
    R_CachePatch(&m_htic, "M_HTIC");
    R_CachePatch(&dpFSlot, "M_FSLOT");
#endif
}

#if __JDOOM__ || __JHERETIC__
/**
 * Construct the episode selection menu.
 */
void M_InitEpisodeMenu(void)
{
    int                 i, maxw, w, numEpisodes;

#if __JDOOM__
    switch(gameMode)
    {
    case commercial:    numEpisodes = 0; break;
    case retail:        numEpisodes = 4; break;
    // In shareware, episodes 2 and 3 are handled, branching to an ad screen.
    default:            numEpisodes = 3; break;
    }
#else  __JHERETIC__
    if(gameMode == extended)
        numEpisodes = 6;
    else
        numEpisodes = 3;
#endif

    // Allocate the menu items array.
    EpisodeItems = Z_Calloc(sizeof(menuitem_t) * numEpisodes, PU_STATIC, 0);

    for(i = 0, maxw = 0; i < numEpisodes; ++i)
    {
        menuitem_t*             item = &EpisodeItems[i];

        item->type = ITT_EFUNC;
        item->func = M_Episode;
        item->option = i;
        item->text = GET_TXT(TXT_EPISODE1 + i);
        w = M_StringWidth(item->text, EpiDef.font);
        if(w > maxw)
            maxw = w;
# if __JDOOM__
        item->patch = &episodeNamePatches[i];
# endif
    }

    // Finalize setup.
    EpiDef.items = EpisodeItems;
    EpiDef.itemCount = numEpisodes;
    EpiDef.numVisItems = MIN_OF(EpiDef.itemCount, 10);
    EpiDef.x = 160 - maxw / 2 + 12; // Center the menu appropriately.
}
#endif

#if __JHEXEN__
/**
 * Construct the player class selection menu.
 */
void M_InitPlayerClassMenu(void)
{
    uint                i, n, count;

    // First determine the number of selectable player classes.
    count = 0;
    for(i = 0; i < NUM_PLAYER_CLASSES; ++i)
    {
        classinfo_t*        info = PCLASS_INFO(i);

        if(info->userSelectable)
            count++;
    }

    // Allocate the menu items array.
    ClassItems = Z_Calloc(sizeof(menuitem_t) * (count + 1),
                          PU_STATIC, 0);

    // Add the selectable classes.
    n = i = 0;
    while(n < count)
    {
        classinfo_t*        info = PCLASS_INFO(i++);
        menuitem_t*         item;

        if(!info->userSelectable)
            continue;

        item = &ClassItems[n];
        item->type = ITT_EFUNC;
        item->func = M_ChooseClass;
        item->option = n;
        item->text = info->niceName;

        n++;
    }

    // Add the random class option.
    ClassItems[n].type = ITT_EFUNC;
    ClassItems[n].func = M_ChooseClass;
    ClassItems[n].option = -1;
    ClassItems[n].text = GET_TXT(TXT_RANDOMPLAYERCLASS);

    // Finalize setup.
    ClassDef.items = ClassItems;
    ClassDef.itemCount = count + 1;
    ClassDef.numVisItems = MIN_OF(ClassDef.itemCount, 10);
}
#endif

/**
 * Menu initialization.
 * Called during (post-engine) init and after updating game/engine state.
 *
 * Initializes the various vars, fonts, adjust the menu structs and
 * anything else that needs to be done before the menu can be used.
 */
void Hu_MenuInit(void)
{
#if !__JDOOM64__
    menuitem_t *item;
#endif
#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    int   i, w, maxw;
#endif
#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    R_GetGammaMessageStrings();
#endif

#if __JDOOM__ || __JDOOM64__
    // Quit messages.
    endmsg[0] = GET_TXT(TXT_QUITMSG);
    for(i = 1; i <= NUM_QUITMESSAGES; ++i)
        endmsg[i] = GET_TXT(TXT_QUITMESSAGE1 + i - 1);
#endif

#if __JDOOM__ || __JDOOM64__
    // Skill names.
    for(i = 0, maxw = 0; i < NUM_SKILL_MODES; ++i)
    {
        SkillItems[i].text = GET_TXT(TXT_SKILL1 + i);
        w = M_StringWidth(SkillItems[i].text, SkillDef.font);
        if(w > maxw)
            maxw = w;
    }
    // Center the skill menu appropriately.
    SkillDef.x = 160 - maxw / 2 + 12;
#endif

    // Play modes.
    NewGameItems[0].text = GET_TXT(TXT_SINGLEPLAYER);
    NewGameItems[1].text = GET_TXT(TXT_MULTIPLAYER);

    currentMenu = &MainDef;
    menuActive = false;
    DD_Execute(true, "deactivatebcontext menu");
    menuAlpha = menuTargetAlpha = 0;

    M_LoadData();

    itemOn = currentMenu->lastOn;
    whichSkull = 0;
    skullAnimCounter = MENUCURSOR_TICSPERFRAME;
    quickSaveSlot = -1;

#if __JDOOM__
    // Here we catch version dependencies, like HELP1/2, and four episodes.
    switch(gameMode)
    {
    case commercial:
        item = &MainItems[4]; // Read This!
        item->func = M_QuitDOOM;
        item->text = "{case}Quit Game";
        item->patch = &m_quitg;
        MainDef.itemCount--;
        MainDef.y += 8;
        SkillDef.prevMenu = MENU_NEWGAME;
        ReadDef1.x = 330;
        ReadDef1.y = 165;
        ReadDef1.background = "HELP";
        ReadDef1.backgroundIsRaw = false;
        ReadDef2.background = "CREDIT";
        ReadDef2.backgroundIsRaw = false;
        ReadItems1[0].func = M_FinishReadThis;
        break;
    case shareware:
        // Episode 2 and 3 are handled, branching to an ad screen.
    case registered:
        ReadDef1.background = "HELP1";
        ReadDef1.backgroundIsRaw = false;
        ReadDef2.background = "HELP2";
        ReadDef2.backgroundIsRaw = false;
        break;
    case retail:
        ReadDef1.background = "HELP1";
        ReadDef2.background = "CREDIT";
        break;

    default:
        break;
    }
#else
# if !__JDOOM64__
    item = &MainItems[READTHISID]; // Read This!
    item->func = M_ReadThis;
# endif
#endif

#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    SkullBaseLump = W_GetNumForName(SKULLBASELMP);
#endif

#if __JDOOM__ || __JHERETIC__
    M_InitEpisodeMenu();
#endif
#if __JHEXEN__
    M_InitPlayerClassMenu();
#endif
    M_InitControlsMenu();
}

/**
 * @return              @c true, iff the menu is currently active (open).
 */
boolean Hu_MenuIsActive(void)
{
    return menuActive;
}

/**
 * Set the alpha level of the entire menu.
 *
 * @param alpha         Alpha level to set the menu too (0...1)
 */
void Hu_MenuSetAlpha(float alpha)
{
    // The menu's alpha will start moving towards this target value.
    menuTargetAlpha = alpha;
}

/**
 * @return              Current alpha level of the menu.
 */
float Hu_MenuAlpha(void)
{
    return menuAlpha;
}

/**
 * Updates on Game Tick.
 */
void Hu_MenuTicker(timespan_t time)
{
#define MENUALPHA_FADE_STEP (.07f)

    float               diff;
    static trigger_t    fixed = { 1 / 35.0 };

    if(!M_RunTrigger(&fixed, time))
        return;

    typeInTime++;

    // Move towards the target alpha level for the entire menu.
    diff = menuTargetAlpha - menuAlpha;
    if(fabs(diff) > MENUALPHA_FADE_STEP)
    {
        menuAlpha += MENUALPHA_FADE_STEP * (diff > 0? 1 : -1);
    }
    else
    {
        menuAlpha = menuTargetAlpha;
    }

    if(menuActive || menuAlpha > 0)
    {
        float               rewind = 20;

        // Fade in/out the widget background filter
        if(widgetEdit)
        {
            if(menu_calpha < 0.5f)
                menu_calpha += .1f;
            if(menu_calpha > 0.5f)
                menu_calpha = 0.5f;
        }
        else
        {
            if(menu_calpha > 0)
                menu_calpha -= .1f;
            if(menu_calpha < 0)
                menu_calpha = 0;
        }

        // Animate the cursor patches
        if(--skullAnimCounter <= 0)
        {
            whichSkull++;
            skullAnimCounter = MENUCURSOR_TICSPERFRAME;
            if(whichSkull > cursors-1)
                whichSkull = 0;
        }

        menuTime++;

        menu_color += cfg.flashSpeed;
        if(menu_color >= 100)
            menu_color -= 100;

        if(cfg.turningSkull && currentMenu->items[itemOn].type == ITT_LRFUNC)
            skull_angle += 5;
        else if(skull_angle != 0)
        {
            if(skull_angle <= rewind || skull_angle >= 360 - rewind)
                skull_angle = 0;
            else if(skull_angle < 180)
                skull_angle -= rewind;
            else
                skull_angle += rewind;
        }
        if(skull_angle >= 360)
            skull_angle -= 360;

        // Used for jHeretic's rotating skulls
        frame = (menuTime / 3) % 18;
    }

    if(menuActive)
        MN_TickerEx();

#undef MENUALPHA_FADE_STEP
}

void M_SetupNextMenu(menu_t* menudef)
{
    if(!menudef)
        return;

    currentMenu = menudef;

    // Have we been to this menu before?
    // If so move the cursor to the last selected item
    if(currentMenu->lastOn)
    {
        itemOn = currentMenu->lastOn;
    }
    else
    {   // Select the first active item in this menu.
        int                     i;

        for(i = 0; i < menudef->itemCount; ++i)
        {
            if(menudef->items[i].type != ITT_EMPTY)
                break;
        }

        if(i > menudef->itemCount)
            itemOn = -1;
        else
            itemOn = i;
    }

    menu_color = 0;
    skull_angle = 0;
    typeInTime = 0;
}

/**
 * @return              @c true, if the menu is active and there is a
 *                      background for this page.
 */
boolean MN_CurrentMenuHasBackground(void)
{
    if(!menuActive)
        return false;

    return (currentMenu->background &&
            W_CheckNumForName(currentMenu->background) != -1);
}

/**
 * This is the main menu drawing routine (called every tic by the drawing
 * loop) Draws the current menu 'page' by calling the funcs attached to
 * each menu item.
 */
void Hu_MenuDrawer(void)
{
    int             i;
    int             pos[2], offset[2], width, height;
    float           scale;
    boolean         allowScaling =
        (!(currentMenu->flags & MNF_NOSCALE)? true : false);

    // Popped at the end of the function.
    DGL_MatrixMode(DGL_MODELVIEW);
    DGL_PushMatrix();

    // Setup matrix.
    if(menuActive || menuAlpha > 0)
    {
        // If there is a menu background raw lump, draw it instead of the
        // background effect.
        if(currentMenu->background)
        {
            lumpnum_t           lump =
                W_CheckNumForName(currentMenu->background);

            if(lump != -1)
            {
                DGL_Color4f(1, 1, 1, menuAlpha);

                if(currentMenu->backgroundIsRaw)
                    GL_DrawRawScreen_CS(lump, 0, 0, 1, 1);
                else
                    GL_DrawPatch_CS(0, 0, lump);
            }
        }

        // Allow scaling?
        if(allowScaling)
        {
            // Scale by the menuScale.
            DGL_MatrixMode(DGL_MODELVIEW);

            DGL_Translatef(160, 100, 0);
            DGL_Scalef(cfg.menuScale, cfg.menuScale, 1);
            DGL_Translatef(-160, -100, 0);
        }
    }

    if(!menuActive && !(menuAlpha > 0))
        goto end_draw_menu;

    if(allowScaling && currentMenu->unscaled.numVisItems)
    {
        currentMenu->numVisItems = currentMenu->unscaled.numVisItems / cfg.menuScale;
        currentMenu->y = 110 - (110 - currentMenu->unscaled.y) / cfg.menuScale;

        if(currentMenu->firstItem && currentMenu->firstItem < currentMenu->numVisItems)
        {
            // Make sure all pages are divided correctly.
            currentMenu->firstItem = 0;
        }
        if(itemOn - currentMenu->firstItem >= currentMenu->numVisItems)
        {
            itemOn = currentMenu->firstItem + currentMenu->numVisItems - 1;
        }
    }

    if(currentMenu->drawFunc)
        currentMenu->drawFunc(); // Call Draw routine.

    pos[VX] = currentMenu->x;
    pos[VY] = currentMenu->y;

    if(menuAlpha > 0.0125f)
    {
        for(i = currentMenu->firstItem;
            i < currentMenu->itemCount && i < currentMenu->firstItem + currentMenu->numVisItems; ++i)
        {
            float           t, r, g, b;

            // Which color?
#if __JDOOM__ || __JDOOM64__
            if(!cfg.usePatchReplacement)
            {
                r = 1;
                g = b = 0;
            }
            else
            {
#endif
            if(currentMenu->items[i].type == ITT_EMPTY ||
               currentMenu->items[i].type >= ITT_INERT)
            {
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
                r = cfg.menuColor[0];
                g = cfg.menuColor[1];
                b = cfg.menuColor[2];
#else
                // FIXME
                r = 1;
                g = .7f;
                b = .3f;
#endif
            }
            else if(itemOn == i && !widgetEdit && cfg.usePatchReplacement)
            {
                // Selection!
                if(menu_color <= 50)
                    t = menu_color / 50.0f;
                else
                    t = (100 - menu_color) / 50.0f;
                r = currentMenu->color[0] * t + cfg.flashColor[0] * (1 - t);
                g = currentMenu->color[1] * t + cfg.flashColor[1] * (1 - t);
                b = currentMenu->color[2] * t + cfg.flashColor[2] * (1 - t);
            }
            else
            {
                r = currentMenu->color[0];
                g = currentMenu->color[1];
                b = currentMenu->color[2];
            }
#if __JDOOM__ || __JDOOM64__
            }
#endif
            if(currentMenu->items[i].patch)
            {
                WI_DrawPatch(pos[VX], pos[VY], r, g, b, menuAlpha,
                             currentMenu->items[i].patch,
                             (currentMenu->items[i].flags & MIF_NOTALTTXT)? NULL :
                             currentMenu->items[i].text, true, ALIGN_LEFT);
            }
            else if(currentMenu->items[i].text)
            {
                WI_DrawParamText(pos[VX], pos[VY],
                                 currentMenu->items[i].text, currentMenu->font,
                                 r, g, b, menuAlpha,
                                 false,
                                 cfg.usePatchReplacement? true : false,
                                 ALIGN_LEFT);
            }

            pos[VY] += currentMenu->itemHeight;
        }

        // Draw the colour widget?
        if(widgetEdit)
        {
            Draw_BeginZoom(0.5f, 160, 100);
            DrawColorWidget();
        }

        // Draw the menu cursor.
        if(allowScaling)
        {
            if(itemOn >= 0)
            {
                menu_t* mn = (widgetEdit? &ColorWidgetMnu : currentMenu);

                scale = mn->itemHeight / (float) LINEHEIGHT;
                width = cursorst[whichSkull].width * scale;
                height = cursorst[whichSkull].height * scale;

                offset[VX] = mn->x;
                offset[VX] += MENUCURSOR_OFFSET_X * scale;

                offset[VY] = mn->y;
                offset[VY] += (itemOn - mn->firstItem + 1) * mn->itemHeight;
                offset[VY] -= (float) height / 2;
                offset[VY] += MENUCURSOR_OFFSET_Y * scale;

                GL_SetPatch(cursorst[whichSkull].lump, DGL_CLAMP, DGL_CLAMP);

                DGL_MatrixMode(DGL_MODELVIEW);
                DGL_PushMatrix();

                DGL_Translatef(offset[VX], offset[VY], 0);
                DGL_Scalef(1, 1.0f / 1.2f, 1);
                if(skull_angle)
                    DGL_Rotatef(skull_angle, 0, 0, 1);
                DGL_Scalef(1, 1.2f, 1);

                GL_DrawRect(-width/2.f, -height/2.f, width, height, 1, 1, 1, menuAlpha);

                DGL_MatrixMode(DGL_MODELVIEW);
                DGL_PopMatrix();
            }
        }

        if(widgetEdit)
        {
            Draw_EndZoom();
        }
    }

  end_draw_menu:

    // Restore original matrices.
    DGL_MatrixMode(DGL_MODELVIEW);
    DGL_PopMatrix();

    M_ControlGrabDrawer();
}

/**
 * Execute a menu navigation/action command.
 */
void Hu_MenuCommand(menucommand_e cmd)
{
    if(cmd == MCMD_CLOSE || cmd == MCMD_CLOSEFAST)
    {
        Hu_FogEffectSetAlphaTarget(0);

        if(cmd == MCMD_CLOSEFAST)
        {   // Hide the menu instantly.
            menuAlpha = menuTargetAlpha = 0;
        }
        else
        {
            menuTargetAlpha = 0;
        }

        menuActive = false;

        // Disable the menu binding class
        DD_Execute(true, "deactivatebcontext menu");
        return;
    }

    if(!menuActive)
    {
        if(cmd == MCMD_OPEN)
        {
            S_LocalSound(menusnds[2], NULL);

            Con_Open(false);

            Hu_FogEffectSetAlphaTarget(1);
            Hu_MenuSetAlpha(1);
            menuActive = true;
            menu_color = 0;
            menuTime = 0;
            skull_angle = 0;
            currentMenu = &MainDef;
            itemOn = currentMenu->lastOn;
            typeInTime = 0;

            // Enable the menu binding class
            DD_Execute(true, "activatebcontext menu");
        }
    }
    else
    {
        int             i;
        int             firstVI, lastVI; // first and last visible item
        int             itemCountOffset = 0;
        const menuitem_t *item;
        menu_t         *menu = currentMenu;

        if(widgetEdit)
        {
            menu = &ColorWidgetMnu;

            if(!rgba)
                itemCountOffset = 1;
        }

        firstVI = menu->firstItem;
        lastVI = firstVI + menu->numVisItems - 1 - itemCountOffset;
        if(lastVI > menu->itemCount - 1 - itemCountOffset)
            lastVI = menu->itemCount - 1 - itemCountOffset;
        item = &menu->items[itemOn];
        menu->lastOn = itemOn;

        switch(cmd)
        {
        case MCMD_NAV_LEFT:
            if(item->type == ITT_LRFUNC && item->func != NULL)
            {
                item->func(LEFT_DIR | item->option, item->data);
                S_LocalSound(menusnds[4], NULL);
            }
            else
            {
                menu_nav_left:

                // Let's try to change to the previous page.
                if(menu->firstItem - menu->numVisItems >= 0)
                {
                    menu->firstItem -= menu->numVisItems;
                    itemOn -= menu->numVisItems;

                    // Ensure cursor points to an editable item
                    firstVI = menu->firstItem;
                    while(menu->items[itemOn].type == ITT_EMPTY &&
                          (itemOn > firstVI))
                        itemOn--;

                    while(!menu->items[itemOn].type &&
                         itemOn < menu->numVisItems)
                        itemOn++;

                    // Make a sound, too.
                    S_LocalSound(menusnds[4], NULL);
                }
            }
            break;

        case MCMD_NAV_RIGHT:
            if(item->type == ITT_LRFUNC && item->func != NULL)
            {
                item->func(RIGHT_DIR | item->option, item->data);
                S_LocalSound(menusnds[4], NULL);
            }
            else
            {
                menu_nav_right:

                // Move on to the next page, if possible.
                if(menu->firstItem + menu->numVisItems <
                   menu->itemCount)
                {
                    menu->firstItem += menu->numVisItems;
                    itemOn += menu->numVisItems;

                    // Ensure cursor points to an editable item
                    firstVI = menu->firstItem;
                    while((menu->items[itemOn].type == ITT_EMPTY ||
                           itemOn >= menu->itemCount) && itemOn > firstVI)
                        itemOn--;

                    while(menu->items[itemOn].type == ITT_EMPTY &&
                          itemOn < menu->numVisItems)
                        itemOn++;

                    // Make a sound, too.
                    S_LocalSound(menusnds[4], NULL);
                }
            }
            break;

        case MCMD_NAV_DOWN:
            i = 0;
            do
            {
                if(itemOn + 1 > lastVI)
                    itemOn = firstVI;
                else
                    itemOn++;
            } while(menu->items[itemOn].type == ITT_EMPTY &&
                    i++ < menu->itemCount);
            menu_color = 0;
            S_LocalSound(menusnds[3], NULL);
            break;

        case MCMD_NAV_UP:
            i = 0;
            do
            {
                if(itemOn <= firstVI)
                    itemOn = lastVI;
                else
                    itemOn--;
            } while(menu->items[itemOn].type == ITT_EMPTY &&
                    i++ < menu->itemCount);
            menu_color = 0;
            S_LocalSound(menusnds[3], NULL);
            break;

        case MCMD_NAV_OUT:
            menu->lastOn = itemOn;
            if(menu->prevMenu == MENU_NONE)
            {
                menu->lastOn = itemOn;
                S_LocalSound(menusnds[1], NULL);
                Hu_MenuCommand(MCMD_CLOSE);
            }
            else
            {
                M_SetupNextMenu(menulist[menu->prevMenu]);
                S_LocalSound(menusnds[2], NULL);
            }
            break;

        case MCMD_DELETE:
            if(menu->flags & MNF_DELETEFUNC)
            {
                if(item->func)
                {
                    item->func(-1, item->data);
                    S_LocalSound(menusnds[2], NULL);
                }
            }
            break;

        case MCMD_SELECT:
            if(item->type == ITT_SETMENU)
            {
                M_SetupNextMenu(menulist[item->option]);
                S_LocalSound(menusnds[5], NULL);
            }
            else if(item->type == ITT_NAVLEFT)
            {
                goto menu_nav_left;
            }
            else if(item->type == ITT_NAVRIGHT)
            {
                goto menu_nav_right;
            }
            else if(item->func != NULL)
            {
                menu->lastOn = itemOn;
                if(item->type == ITT_LRFUNC)
                {
                    item->func(RIGHT_DIR | item->option, item->data);
                    S_LocalSound(menusnds[5], NULL);
                }
                else if(item->type == ITT_EFUNC)
                {
                    item->func(item->option, item->data);
                    S_LocalSound(menusnds[5], NULL);
                }
            }
            break;
        }
    }
}

/**
 * Responds to alphanumeric input for edit fields.
 */
boolean M_EditResponder(event_t *ev)
{
    int                 ch = -1;
    char*               ptr;

    if(!saveStringEnter && !ActiveEdit/* && !messageToPrint*/)
        return false;

    if(ev->data1 == DDKEY_RSHIFT)
        shiftdown = (ev->state == EVS_DOWN);

    if(ev->type == EV_KEY && (ev->state == EVS_DOWN || ev->state == EVS_REPEAT))
        ch = ev->data1;

    if(ch == -1)
        return false;

    // String input
    if(saveStringEnter || ActiveEdit)
    {
        ch = toupper(ch);
        if(!(ch != 32 && (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE)))
        {
            if(ch >= ' ' && ch <= 'Z')
            {
                if(shiftdown && shiftTable[ch - 32])
                    ch = shiftTable[ch - 32];

                if(saveStringEnter)
                {
                    if(saveCharIndex < SAVESTRINGSIZE &&
                        M_StringWidth(savegamestrings[saveSlot], huFontA)
                        < (SAVESTRINGSIZE - 1) * 8)
                    {
                        savegamestrings[saveSlot][saveCharIndex++] = ch;
                        savegamestrings[saveSlot][saveCharIndex] = 0;
                    }
                }
                else
                {
                    if(strlen(ActiveEdit->text) < MAX_EDIT_LEN - 2)
                    {
                        ptr = ActiveEdit->text + strlen(ActiveEdit->text);
                        ptr[0] = ch;
                        ptr[1] = 0;
                        Ed_MakeCursorVisible();
                    }
                }
            }
            return true;
        }
    }

    return false;
}

/**
 * This is the "fallback" responder, its the last stage in the event chain
 * so if an event reaches here it means there was no suitable binding for it.
 *
 * Handles the hotkey selection in the menu and "press any key" messages.
 *
 * @return              @c true, if it ate the event.
 */
boolean Hu_MenuResponder(event_t* ev)
{
    int                 ch = -1;
    int                 i;
    uint                cid;
    int                 firstVI, lastVI;    // first and last visible item
    boolean             skip;

    // Handle "Press any key to continue" messages
    if(Hu_MsgResponder(ev))
        return true;

    if(!menuActive)
    {
        // Any key/button down pops up menu if in demos.
        if(G_GetGameAction() == GA_NONE && !singledemo &&
           (Get(DD_PLAYBACK) || FI_IsMenuTrigger(ev)))
        {
            if(ev->state == EVS_DOWN &&
               (ev->type == EV_KEY || ev->type == EV_MOUSE_BUTTON ||
                ev->type == EV_JOY_BUTTON))
            {
                Hu_MenuCommand(MCMD_OPEN);
                return true;
            }
        }

        return false;
    }

    if(widgetEdit || (currentMenu->flags & MNF_NOHOTKEYS))
        return false;

    if(ev->type == EV_KEY && (ev->state == EVS_DOWN || ev->state == EVS_REPEAT))
        ch = ev->data1;

    if(ch == -1)
        return false;

    firstVI = currentMenu->firstItem;
    lastVI = firstVI + currentMenu->numVisItems - 1;

    if(lastVI > currentMenu->itemCount - 1)
        lastVI = currentMenu->itemCount - 1;
    currentMenu->lastOn = itemOn;

    // First letter of each item is treated as a hotkey
    for(i = firstVI; i <= lastVI; ++i)
    {
        if(currentMenu->items[i].text && currentMenu->items[i].type != ITT_EMPTY)
        {
            cid = 0;
            if(currentMenu->items[i].text[0] == '{')
            {   // A parameter string, skip till '}' is found
                for(cid = 0, skip = true;
                     cid < strlen(currentMenu->items[i].text) && skip; ++cid)
                    if(currentMenu->items[i].text[cid] == '}')
                        skip = false;
            }

            if(toupper(ch) == toupper(currentMenu->items[i].text[cid]))
            {
                itemOn = i;
                return true;
            }
        }
    }

    return false;
}

/**
 * The colour widget edits the "hot" currentcolour[]
 * The widget responder handles setting the specified vars to that of the
 * currentcolour.
 *
 * \fixme The global value rgba (fixme!) is used to control if rgb or rgba input
 * is needed, as defined in the widgetcolors array.
 */
void DrawColorWidget(void)
{
    int         w = 0;
    menu_t     *menu = &ColorWidgetMnu;

    if(widgetEdit)
    {
#if __JDOOM__ || __JDOOM64__
        w = 38;
#else
        w = 46;
#endif

        M_DrawBackgroundBox(menu->x -30, menu->y -40,
#if __JDOOM__ || __JDOOM64__
                        160, (rgba? 85 : 75),
#else
                        180, (rgba? 170 : 140),
#endif
                             1, 1, 1, menuAlpha, true, BORDERUP);

        GL_SetNoTexture();
        GL_DrawRect(menu->x+w, menu->y-30, 24, 22, currentcolor[0],
                    currentcolor[1], currentcolor[2], currentcolor[3]);
        M_DrawBackgroundBox(menu->x+w, menu->y-30, 24, 22, 1, 1, 1,
                            menuAlpha, false, BORDERDOWN);
#if __JDOOM__ || __JDOOM64__
        MN_DrawSlider(menu, 0, 11, currentcolor[0] * 10 + .25f);
        M_WriteText2(menu->x, menu->y, ColorWidgetItems[0].text,
                     huFontA, 1, 1, 1, menuAlpha);
        MN_DrawSlider(menu, 1, 11, currentcolor[1] * 10 + .25f);
        M_WriteText2(menu->x, menu->y + (LINEHEIGHT_A),
                     ColorWidgetItems[1].text, huFontA, 1, 1, 1, menuAlpha);
        MN_DrawSlider(menu, 2, 11, currentcolor[2] * 10 + .25f);
        M_WriteText2(menu->x, menu->y + (LINEHEIGHT_A * 2),
                     ColorWidgetItems[2].text, huFontA, 1, 1, 1, menuAlpha);
#else
        MN_DrawSlider(menu, 1, 11, currentcolor[0] * 10 + .25f);
        M_WriteText2(menu->x, menu->y, ColorWidgetItems[0].text,
                     huFontA, 1, 1, 1, menuAlpha);
        MN_DrawSlider(menu, 4, 11, currentcolor[1] * 10 + .25f);
        M_WriteText2(menu->x, menu->y + (LINEHEIGHT_A * 3),
                     ColorWidgetItems[3].text, huFontA, 1, 1, 1, menuAlpha);
        MN_DrawSlider(menu, 7, 11, currentcolor[2] * 10 + .25f);
        M_WriteText2(menu->x, menu->y + (LINEHEIGHT_A * 6),
                     ColorWidgetItems[6].text, huFontA, 1, 1, 1, menuAlpha);
#endif
        if(rgba)
        {
#if __JDOOM__ || __JDOOM64__
            MN_DrawSlider(menu, 3, 11, currentcolor[3] * 10 + .25f);
            M_WriteText2(menu->x, menu->y + (LINEHEIGHT_A * 3),
                         ColorWidgetItems[3].text, huFontA, 1, 1, 1,
                         menuAlpha);
#else
            MN_DrawSlider(menu, 10, 11, currentcolor[3] * 10 + .25f);
            M_WriteText2(menu->x, menu->y + (LINEHEIGHT_A * 9),
                         ColorWidgetItems[9].text, huFontA, 1, 1, 1,
                         menuAlpha);
#endif
        }

    }
}

/**
 * Inform the menu to activate the color widget
 * An intermediate step. Used to copy the existing rgba values pointed
 * to by the index (these match an index in the widgetcolors array) into
 * the "hot" currentcolor[] slots. Also switches between rgb/rgba input.
 */
void SCColorWidget(int index, void* context)
{
    currentcolor[0] = *widgetcolors[index].r;
    currentcolor[1] = *widgetcolors[index].g;
    currentcolor[2] = *widgetcolors[index].b;

    // Set the index of the colour being edited
    editcolorindex = index;

    // Remember the position of the Skull on the main menu
    previtemOn = itemOn;

    // Set the start position to 0;
    itemOn = 0;

    // Do we want rgb or rgba sliders?
    if(widgetcolors[index].a)
    {
        rgba = true;
        currentcolor[3] = *widgetcolors[index].a;
    }
    else
    {
        rgba = false;
        currentcolor[3] = 1.0f;
    }

    // Activate the widget
    widgetEdit = true;
}

void M_ToggleVar(int index, void* context)
{
    char*               cvarname;

    if(!context)
        return;
    cvarname = (char*) context;

    DD_Executef(true, "toggle %s", cvarname);
    S_LocalSound(menusnds[0], NULL);
}

void M_DrawTitle(char *text, int y)
{
    WI_DrawParamText(160 - M_StringWidth(text, huFontB) / 2, y, text,
                     huFontB, cfg.menuColor[0], cfg.menuColor[1],
                     cfg.menuColor[2], menuAlpha, true, true, ALIGN_LEFT);
}

void M_WriteMenuText(const menu_t* menu, int index, const char* text)
{
    int                 off = 0;

    if(menu->items[index].text)
        off = M_StringWidth(menu->items[index].text, menu->font) + 4;

    M_WriteText2(menu->x + off,
                 menu->y + menu->itemHeight * (index  - menu->firstItem),
                 text, menu->font, 1, 1, 1, menuAlpha);
}

/**
 * User wants to load this game
 */
void M_LoadSelect(int option, void* context)
{
    menu_t*             menu = &SaveDef;
#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    char                name[256];
#endif

    menu->lastOn = option;
    Hu_MenuCommand(MCMD_CLOSEFAST);

#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    SV_GetSaveGameFileName(option, name);
    G_LoadGame(name);
#else
    G_LoadGame(option);
#endif
}

/**
 * User wants to save. Start string input for Hu_MenuResponder
 */
void M_SaveSelect(int option, void* context)
{
    menu_t*             menu = &LoadDef;

    // we are going to be intercepting all chars
    saveStringEnter = 1;

    menu->lastOn = saveSlot = option;
    strcpy(saveOldString, savegamestrings[option]);
    if(!strcmp(savegamestrings[option], EMPTYSTRING))
        savegamestrings[option][0] = 0;
    saveCharIndex = strlen(savegamestrings[option]);
}

void M_DrawMainMenu(void)
{
#if __JHEXEN__
    int         frame;

    frame = (menuTime / 5) % 7;

    DGL_Color4f( 1, 1, 1, menuAlpha);
    GL_DrawPatch_CS(88, 0, m_htic.lump);
    GL_DrawPatch_CS(37, 80, SkullBaseLump + (frame + 2) % 7);
    GL_DrawPatch_CS(278, 80, SkullBaseLump + frame);

#elif __JHERETIC__
    WI_DrawPatch(88, 0, 1, 1, 1, menuAlpha, &m_htic, NULL, false,
                 ALIGN_LEFT);

    DGL_Color4f( 1, 1, 1, menuAlpha);
    GL_DrawPatch_CS(40, 10, SkullBaseLump + (17 - frame));
    GL_DrawPatch_CS(232, 10, SkullBaseLump + frame);
#elif __JDOOM__ || __JDOOM64__
    WI_DrawPatch(94, 2, 1, 1, 1, menuAlpha, &m_doom,
                 NULL, false, ALIGN_LEFT);
#elif __JSTRIFE__
    menu_t     *menu = &MainDef;
    int         yoffset = 0;

    WI_DrawPatch(86, 2, 1, 1, 1, menuAlpha, W_GetNumForName("M_STRIFE"),
                 NULL, false, ALIGN_LEFT);

    WI_DrawPatch(menu->x, menu->y + yoffset, 1, 1, 1, menuAlpha,
                 W_GetNumForName("M_NGAME"), NULL, false, ALIGN_LEFT);
    WI_DrawPatch(menu->x, menu->y + (yoffset+= menu->itemHeight), 1, 1, 1,
                 menuAlpha, W_GetNumForName("M_NGAME"), NULL, false, ALIGN_LEFT);
    WI_DrawPatch(menu->x, menu->y + (yoffset+= menu->itemHeight), 1, 1, 1,
                 menuAlpha, W_GetNumForName("M_OPTION"), NULL, false, ALIGN_LEFT);
    WI_DrawPatch(menu->x, menu->y + (yoffset+= menu->itemHeight), 1, 1, 1,
                 menuAlpha, W_GetNumForName("M_LOADG"), NULL, false, ALIGN_LEFT);
    WI_DrawPatch(menu->x, menu->y + (yoffset+= menu->itemHeight), 1, 1, 1,
                 menuAlpha, W_GetNumForName("M_SAVEG"), NULL, false, ALIGN_LEFT);
    WI_DrawPatch(menu->x, menu->y + (yoffset+= menu->itemHeight), 1, 1, 1,
                 menuAlpha, W_GetNumForName("M_RDTHIS"), NULL, false, ALIGN_LEFT);
    WI_DrawPatch(menu->x, menu->y + (yoffset+= menu->itemHeight), 1, 1, 1,
                 menuAlpha, W_GetNumForName("M_QUITG"), NULL, false, ALIGN_LEFT);
#endif
}

void M_DrawNewGameMenu(void)
{
    menu_t*             menu = &NewGameDef;
    M_DrawTitle(GET_TXT(TXT_PICKGAMETYPE), menu->y - 30);
}

void M_DrawReadThis(void)
{
#if __JDOOM__
    // The background is handled elsewhere, just draw the cursor.
    GL_DrawPatch(298, 160, cursorst[whichSkull].lump);
#endif
}

static void composeNotDesignedForMessage(const char* str)
{
    char*               buf = notDesignedForMessage, *in, tmp[2];

    buf[0] = 0;
    tmp[1] = 0;

    // Get the message template.
    in = GET_TXT(TXT_NOTDESIGNEDFOR);

    for(; *in; in++)
    {
        if(in[0] == '%')
        {
            if(in[1] == '1')
            {
                strcat(buf, str);
                in++;
                continue;
            }

            if(in[1] == '%')
                in++;
        }
        tmp[0] = *in;
        strcat(buf, tmp);
    }
}

#if __JHEXEN__
void M_DrawClassMenu(void)
{
#define BG_X            (174)
#define BG_Y            (8)

    menu_t*             menu = &ClassDef;
    playerclass_t       pClass;
    spriteinfo_t        sprInfo;
    int                 color;
    static char* boxLumpName[3] = {
        "m_fbox",
        "m_cbox",
        "m_mbox"
    };

    M_WriteText2(34, 24, "CHOOSE CLASS:", huFontB, menu->color[0],
                 menu->color[1], menu->color[2], menuAlpha);

    pClass = (playerclass_t) menu->items[itemOn].option;
    if(pClass < 0)
    {   // Random class.
        // Number of user-selectable classes.
        pClass = (menuTime / 5) % (menu->itemCount - 1);
    }

    R_GetSpriteInfo(states[PCLASS_INFO(pClass)->normalState].sprite,
                    ((menuTime >> 3) & 3), &sprInfo);

    DGL_Color4f(1, 1, 1, menuAlpha);
    GL_DrawPatch_CS(BG_X, BG_Y, W_GetNumForName(boxLumpName[pClass % 3]));

    color = pClass == PCLASS_FIGHTER? 0 : 1;

    if(color)
        GL_SetTranslatedSprite(sprInfo.materialNum, color, pClass);
    else
        GL_SetMaterial(sprInfo.materialNum);

    GL_DrawRect(BG_X + 56 - sprInfo.offset, BG_Y + 78 - sprInfo.topOffset,
                M_CeilPow2(sprInfo.width), M_CeilPow2(sprInfo.height),
                1, 1, 1, menuAlpha);

#undef BG_X
#undef BG_Y
}
#endif

#if __JDOOM__ || __JHERETIC__
void M_DrawEpisode(void)
{
    menu_t*             menu = &EpiDef;

#if __JHERETIC__
    M_DrawTitle("WHICH EPISODE?", 4);

    /**
     * \kludge Inform the user episode 6 is designed for deathmatch only.
     */
    if(menu->items[itemOn].option == 5)
    {
        const char*         str = notDesignedForMessage;

        composeNotDesignedForMessage(GET_TXT(TXT_SINGLEPLAYER));

        M_WriteText2(160 - M_StringWidth(str, huFontA) / 2,
                     200 - M_StringHeight(str, huFontA), str, huFontA,
                     cfg.menuColor2[0], cfg.menuColor2[1], cfg.menuColor2[3],
                     menuAlpha);
    }
#else // __JDOOM__
    WI_DrawPatch(50, 40, menu->color[0], menu->color[1], menu->color[2], menuAlpha,
                 &m_episod, "{case}Which Episode{scaley=1.25,y=-3}?",
                 true, ALIGN_LEFT);
#endif
}
#endif

void M_DrawSkillMenu(void)
{
#if __JHEXEN__ || __JSTRIFE__
    M_DrawTitle("CHOOSE SKILL LEVEL:", 16);
#elif __JHERETIC__
    M_DrawTitle("SKILL LEVEL?", 4);
#elif __JDOOM__ || __JDOOM64__
    menu_t *menu = &SkillDef;
    WI_DrawPatch(96, 14, menu->color[0], menu->color[1], menu->color[2], menuAlpha,
                 &m_newg, "{case}NEW GAME", true, ALIGN_LEFT);
    WI_DrawPatch(54, 38, menu->color[0], menu->color[1], menu->color[2], menuAlpha,
                 &m_skill, "{case}Choose Skill Level:", true,
                 ALIGN_LEFT);
#endif
}

void M_DrawFilesMenu(void)
{
    // clear out the quicksave/quickload stuff
    quicksave = 0;
    quickload = 0;
}

/**
 * Read the strings from the savegame files.
 */
static boolean readSaveString(char* str, size_t len, filename_t fileName)
{
    if(!SV_GetSaveDescription(fileName, str))
    {
        strncpy(str, EMPTYSTRING, len);
        return false;
    }

    return true;
}

static void updateSaveList(void)
{
    int                 i;
    filename_t          fileName;

    for(i = 0; i < NUMSAVESLOTS; ++i)
    {
        menuitem_t*         loadSlot = &LoadItems[i];

        SV_GetSaveGameFileName(i, fileName);

        memset(savegamestrings[i], 0, SAVESTRINGSIZE);
        if(readSaveString(savegamestrings[i], SAVESTRINGSIZE, fileName))
        {
            loadSlot->type = ITT_EFUNC;
        }
        else
        {
            loadSlot->type = ITT_EMPTY;
        }
    }
}

#if __JDOOM__ || __JDOOM64__
#define SAVEGAME_BOX_YOFFSET 3
#else
#define SAVEGAME_BOX_YOFFSET 5
#endif

void M_DrawLoad(void)
{
    int                 i;
    menu_t*             menu = &LoadDef;
    float               t, r, g, b;
    int                 width =
        M_StringWidth("a", menu->font) * (SAVESTRINGSIZE - 1);

#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    M_DrawTitle("LOAD GAME", 4);
#else
    WI_DrawPatch(72, 24, menu->color[0], menu->color[1], menu->color[2], menuAlpha,
                 &m_loadg, "{case}LOAD GAME", true, ALIGN_LEFT);
#endif

    if(menu_color <= 50)
        t = menu_color / 50.0f;
    else
        t = (100 - menu_color) / 50.0f;
    r = currentMenu->color[0] * t + cfg.flashColor[0] * (1 - t);
    g = currentMenu->color[1] * t + cfg.flashColor[1] * (1 - t);
    b = currentMenu->color[2] * t + cfg.flashColor[2] * (1 - t);

    for(i = 0; i < NUMSAVESLOTS; ++i)
    {
        M_DrawSaveLoadBorder(LoadDef.x - 8, SAVEGAME_BOX_YOFFSET + LoadDef.y +
                             (menu->itemHeight * i), width + 16);

        M_WriteText2(LoadDef.x, SAVEGAME_BOX_YOFFSET + LoadDef.y + 1 +
                     (menu->itemHeight * i),
                     savegamestrings[i], menu->font,
                     i == itemOn? r : menu->color[0],
                     i == itemOn? g : menu->color[1],
                     i == itemOn? b : menu->color[2], menuAlpha);
    }
}

void M_DrawSave(void)
{
    int                 i;
    menu_t*             menu = &SaveDef;
    float               t, r, g, b;
    int                 width =
        M_StringWidth("a", menu->font) * (SAVESTRINGSIZE - 1);

#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    M_DrawTitle("SAVE GAME", 4);
#else
    WI_DrawPatch(72, 24, menu->color[0], menu->color[1], menu->color[2], menuAlpha,
                 &m_saveg, "{case}SAVE GAME", true, ALIGN_LEFT);
#endif

    if(menu_color <= 50)
        t = menu_color / 50.0f;
    else
        t = (100 - menu_color) / 50.0f;
    r = currentMenu->color[0] * t + cfg.flashColor[0] * (1 - t);
    g = currentMenu->color[1] * t + cfg.flashColor[1] * (1 - t);
    b = currentMenu->color[2] * t + cfg.flashColor[2] * (1 - t);

    for(i = 0; i < NUMSAVESLOTS; ++i)
    {
        M_DrawSaveLoadBorder(SaveDef.x - 8, SAVEGAME_BOX_YOFFSET + SaveDef.y +
                             (menu->itemHeight * i), width + 16);

        M_WriteText2(SaveDef.x, SAVEGAME_BOX_YOFFSET + SaveDef.y + 1 +
                     (menu->itemHeight * i),
                     savegamestrings[i], menu->font,
                     i == itemOn? r : menu->color[0],
                     i == itemOn? g : menu->color[1],
                     i == itemOn? b : menu->color[2], menuAlpha);
    }

    if(saveStringEnter)
    {
        size_t              len = strlen(savegamestrings[saveSlot]);

        if(len < SAVESTRINGSIZE)
        {
            i = M_StringWidth(savegamestrings[saveSlot], huFontA);
            M_WriteText2(SaveDef.x + i, SAVEGAME_BOX_YOFFSET + SaveDef.y + 1 +
                         (menu->itemHeight * saveSlot), "_", huFontA,
                         r, g, b, menuAlpha);
        }
    }
}

/**
 * Draw border for the savegame description
 */
void M_DrawSaveLoadBorder(int x, int y, int width)
{
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    DGL_Color4f(1, 1, 1, menuAlpha);
    GL_DrawPatch_CS(x - 8, y - 4, dpFSlot.lump);
#else
    DGL_Color4f(1, 1, 1, menuAlpha);

    GL_SetPatch(dpLSLeft.lump, DGL_CLAMP, DGL_CLAMP);
    GL_DrawRect(x, y - 3, dpLSLeft.width, dpLSLeft.height, 1, 1, 1, menuAlpha);
    GL_SetPatch(dpLSRight.lump, DGL_CLAMP, DGL_CLAMP);
    GL_DrawRect(x + width - dpLSRight.width, y - 3, dpLSRight.width,
                dpLSRight.height, 1, 1, 1, menuAlpha);

    GL_SetPatch(dpLSCntr.lump, DGL_REPEAT, DGL_REPEAT);
    GL_DrawRectTiled(x + dpLSLeft.width, y - 3,
                     width - dpLSLeft.width - dpLSRight.width,
                     14, 8, 14);
#endif
}

/**
 * \fixme Hu_MenuResponder calls this when user is finished.
 * not in jHexen it doesn't...
 */
void M_DoSave(int slot)
{
    G_SaveGame(slot, savegamestrings[slot]);
    Hu_MenuCommand(MCMD_CLOSEFAST);

    // Picked a quicksave slot yet?
    if(quickSaveSlot == -2)
        quickSaveSlot = slot;
}

int M_QuickSaveResponse(msgresponse_t response, void* context)
{
    if(response == MSG_YES)
    {
        M_DoSave(quickSaveSlot);
    }

    return true;
}

/**
 * Called via the bindings mechanism when a player wishes to save their
 * game to a preselected save slot.
 */
static void M_QuickSave(void)
{
    player_t*               player = &players[CONSOLEPLAYER];

    if(player->playerState == PST_DEAD ||
       Get(DD_PLAYBACK))
    {
        Hu_MsgStart(MSG_ANYKEY, SAVEDEAD, NULL, NULL);
        return;
    }

    if(G_GetGameState() != GS_MAP)
    {
        Hu_MsgStart(MSG_ANYKEY, SAVEOUTMAP, NULL, NULL);
        return;
    }

    if(quickSaveSlot < 0)
    {
        Hu_MenuCommand(MCMD_OPEN);
        updateSaveList();
        M_SetupNextMenu(&SaveDef);
        quickSaveSlot = -2; // Means to pick a slot now.
        return;
    }
    sprintf(tempstring, QSPROMPT, savegamestrings[quickSaveSlot]);

    if(!cfg.askQuickSaveLoad)
    {
        M_DoSave(quickSaveSlot);
        S_LocalSound(menusnds[1], NULL);
        return;
    }

    Hu_MsgStart(MSG_YESNO, tempstring, M_QuickSaveResponse, NULL);
}

int M_QuickLoadResponse(msgresponse_t response, void* context)
{
    if(response == MSG_YES)
    {
        M_LoadSelect(quickSaveSlot, NULL);
    }

    return true;
}

static void M_QuickLoad(void)
{
    if(IS_NETGAME)
    {
        Hu_MsgStart(MSG_ANYKEY, QLOADNET, NULL, NULL);
        return;
    }

    if(quickSaveSlot < 0)
    {
        Hu_MsgStart(MSG_ANYKEY, QSAVESPOT, NULL, NULL);
        return;
    }

    sprintf(tempstring, QLPROMPT, savegamestrings[quickSaveSlot]);

    if(!cfg.askQuickSaveLoad)
    {
        M_LoadSelect(quickSaveSlot, NULL);
        S_LocalSound(menusnds[1], NULL);
        return;
    }

    Hu_MsgStart(MSG_YESNO, tempstring, M_QuickLoadResponse, NULL);
}

#if !__JDOOM64__
void M_ReadThis(int option, void* context)
{
    option = 0;
    M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int option, void* context)
{
    option = 0;
    M_SetupNextMenu(&ReadDef2);
}

# if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
void M_ReadThis3(int option, void* context)
{
    option = 0;
    M_SetupNextMenu(&ReadDef3);
}
# endif

void M_FinishReadThis(int option, void* context)
{
    option = 0;
    M_SetupNextMenu(&MainDef);
}
#endif

void M_DrawOptions(void)
{
    menu_t*             menu = &OptionsDef;

#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    M_DrawTitle("OPTIONS", menu->y - 32);
#else
# if __JDOOM64__
    WI_DrawPatch(160, menu->y - 20, cfg.menuColor[0], cfg.menuColor[1],
                 cfg.menuColor[2], menuAlpha, 0, "{case}OPTIONS", true,
                 ALIGN_CENTER);
#else
    WI_DrawPatch(160, menu->y - 20, cfg.menuColor[0], cfg.menuColor[1],
                 cfg.menuColor[2], menuAlpha, &m_optttl, "{case}OPTIONS", true,
                 ALIGN_CENTER);
# endif
#endif
}

void M_DrawOptions2(void)
{
    menu_t*             menu = &Options2Def;

#if __JDOOM__ || __JDOOM64__
    M_DrawTitle("SOUND OPTIONS", menu->y - 20);

    MN_DrawSlider(menu, 0, 16, SFXVOLUME);
    MN_DrawSlider(menu, 1, 16, MUSICVOLUME);
#else
    M_DrawTitle("SOUND OPTIONS", 0);

    MN_DrawSlider(menu, 1, 16, SFXVOLUME);
    MN_DrawSlider(menu, 4, 16, MUSICVOLUME);
#endif
}

void M_DrawGameplay(void)
{
    int                 idx = 0;
    menu_t*             menu = &GameplayDef;

#if __JHEXEN__
    M_DrawTitle("GAMEPLAY", 0);
    M_WriteMenuText(menu, idx++, yesno[cfg.alwaysRun != 0]);
    M_WriteMenuText(menu, idx++, yesno[cfg.lookSpring != 0]);
    M_WriteMenuText(menu, idx++, yesno[!cfg.noAutoAim]);
#else

# if __JHERETIC__
    M_DrawTitle("GAMEPLAY", 4);
# else
    M_DrawTitle("GAMEPLAY", menu->y - 20);
# endif

    M_WriteMenuText(menu, idx++, yesno[cfg.alwaysRun != 0]);
    M_WriteMenuText(menu, idx++, yesno[cfg.lookSpring != 0]);
    M_WriteMenuText(menu, idx++, yesno[!cfg.noAutoAim]);
    M_WriteMenuText(menu, idx++, yesno[cfg.jumpEnabled != 0]);
# if __JDOOM64__
    M_WriteMenuText(menu, idx++, yesno[cfg.weaponRecoil != 0]);
    idx = 7;
# else
    idx = 6;
# endif
# if __JDOOM__ || __JDOOM64__
    M_WriteMenuText(menu, idx++, yesno[cfg.anyBossDeath != 0]);
#   if !__JDOOM64__
    M_WriteMenuText(menu, idx++, yesno[cfg.raiseGhosts != 0]);
#   endif
    M_WriteMenuText(menu, idx++, yesno[cfg.maxSkulls != 0]);
    M_WriteMenuText(menu, idx++, yesno[cfg.allowSkullsInWalls != 0]);
# endif
# if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    M_WriteMenuText(menu, idx++, yesno[cfg.monstersStuckInDoors != 0]);
    M_WriteMenuText(menu, idx++, yesno[cfg.avoidDropoffs != 0]);
    M_WriteMenuText(menu, idx++, yesno[cfg.fallOff != 0]);
    M_WriteMenuText(menu, idx++, yesno[cfg.slidingCorpses != 0]);
    M_WriteMenuText(menu, idx++, yesno[cfg.moveBlock != 0]);
    M_WriteMenuText(menu, idx++, yesno[cfg.wallRunNorthOnly != 0]);
# endif
# if __JDOOM__ || __JDOOM64__
    M_WriteMenuText(menu, idx++, yesno[cfg.zombiesCanExit != 0]);
# endif
# if __JDOOM__
    M_WriteMenuText(menu, idx++, yesno[cfg.fixOuchFace != 0]);
# endif
#endif
}

void M_DrawWeaponMenu(void)
{
    menu_t     *menu = &WeaponDef;
    int         i = 0;
    char       *autoswitch[] = { "NEVER", "IF BETTER", "ALWAYS" };
#if __JHEXEN__
    char       *weaponids[] = { "First", "Second", "Third", "Fourth"};
#endif

#if __JDOOM__ || __JDOOM64__
    byte berserkAutoSwitch = cfg.berserkAutoSwitch;
#endif

    M_DrawTitle("WEAPONS", menu->y - 20);

    for(i = 0; i < NUM_WEAPON_TYPES; ++i)
    {
#if __JDOOM__ || __JDOOM64__
        M_WriteMenuText(menu, 4+i, GET_TXT(TXT_WEAPON1 + cfg.weaponOrder[i]));
#elif __JHERETIC__
        /**
         * \fixme We should allow different weapon preferences per player
         * class. However, since the only other class in jHeretic is the
         * chicken which has only 1 weapon anyway -we'll just show the
         * names of the player's weapons for now.
         */
        M_WriteMenuText(menu, 4+i, GET_TXT(TXT_TXT_WPNSTAFF + cfg.weaponOrder[i]));
#elif __JHEXEN__
        /**
         * \fixme We should allow different weapon preferences per player
         * class. Then we can show the real names here.
         */
        M_WriteMenuText(menu, 4+i, weaponids[cfg.weaponOrder[i]]);
#endif
    }

#if __JHEXEN__
    M_WriteMenuText(menu, 8, yesno[cfg.weaponNextMode]);
    M_WriteMenuText(menu, 11, autoswitch[cfg.weaponAutoSwitch]);
    M_WriteMenuText(menu, 12, yesno[cfg.noWeaponAutoSwitchIfFiring]);
    M_WriteMenuText(menu, 13, autoswitch[cfg.ammoAutoSwitch]);
#elif __JHERETIC__
    M_WriteMenuText(menu, 12, yesno[cfg.weaponNextMode]);
    M_WriteMenuText(menu, 15, autoswitch[cfg.weaponAutoSwitch]);
    M_WriteMenuText(menu, 16, yesno[cfg.noWeaponAutoSwitchIfFiring]);
    M_WriteMenuText(menu, 17, autoswitch[cfg.ammoAutoSwitch]);
#elif __JDOOM64__
    M_WriteMenuText(menu, 14, yesno[cfg.weaponNextMode]);
    M_WriteMenuText(menu, 17, autoswitch[cfg.weaponAutoSwitch]);
    M_WriteMenuText(menu, 18, yesno[cfg.noWeaponAutoSwitchIfFiring]);
    M_WriteMenuText(menu, 19, autoswitch[cfg.ammoAutoSwitch]);
    M_WriteMenuText(menu, 20, yesno[berserkAutoSwitch != 0]);
#elif __JDOOM__
    M_WriteMenuText(menu, 13, yesno[cfg.weaponNextMode]);
    M_WriteMenuText(menu, 16, autoswitch[cfg.weaponAutoSwitch]);
    M_WriteMenuText(menu, 17, yesno[cfg.noWeaponAutoSwitchIfFiring]);
    M_WriteMenuText(menu, 18, autoswitch[cfg.ammoAutoSwitch]);
    M_WriteMenuText(menu, 19, yesno[berserkAutoSwitch != 0]);
#endif
}

void M_WeaponOrder(int option, void* context)
{
    int         choice = option >> NUM_WEAPON_TYPES;
    int         temp;

    if(option & RIGHT_DIR)
    {
        if(choice < NUM_WEAPON_TYPES-1)
        {
            temp = cfg.weaponOrder[choice+1];
            cfg.weaponOrder[choice+1] = cfg.weaponOrder[choice];
            cfg.weaponOrder[choice] = temp;

            itemOn++;
        }
    }
    else
    {
        if(choice > 0)
        {
            temp = cfg.weaponOrder[choice];
            cfg.weaponOrder[choice] = cfg.weaponOrder[choice-1];
            cfg.weaponOrder[choice-1] = temp;

            itemOn--;
        }
    }
}

void M_WeaponAutoSwitch(int option, void* context)
{
    if(option == RIGHT_DIR)
    {
        if(cfg.weaponAutoSwitch < 2)
            cfg.weaponAutoSwitch++;
    }
    else if(cfg.weaponAutoSwitch > 0)
        cfg.weaponAutoSwitch--;
}

void M_AmmoAutoSwitch(int option, void* context)
{
    if(option == RIGHT_DIR)
    {
        if(cfg.ammoAutoSwitch < 2)
            cfg.ammoAutoSwitch++;
    }
    else if(cfg.ammoAutoSwitch > 0)
        cfg.ammoAutoSwitch--;
}

void M_DrawHUDMenu(void)
{
#if __JDOOM__ || __JDOOM64__
    int                 idx;
#endif
    menu_t*             menu = &HUDDef;
    char*               xhairnames[7] = {
        "NONE", "CROSS", "ANGLES", "SQUARE", "OPEN SQUARE", "DIAMOND", "V"
    };

#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    char       *token;

    M_DrawTitle("hud options", 4);

    // Draw the page arrows.
    DGL_Color4f( 1, 1, 1, menuAlpha);
    token = (!menu->firstItem || menuTime & 8) ? "invgeml2" : "invgeml1";
    GL_DrawPatch_CS(menu->x -20, menu->y - 16, W_GetNumForName(token));
    token = (menu->firstItem + menu->numVisItems >= menu->itemCount ||
             menuTime & 8) ? "invgemr2" : "invgemr1";
    GL_DrawPatch_CS(312 - (menu->x -20), menu->y - 16, W_GetNumForName(token));
#else
    M_DrawTitle("HUD OPTIONS", menu->y - 20);
#endif

#if __JHEXEN__ || __JSTRIFE__
    if(menu->firstItem < menu->numVisItems)
    {
        M_WriteMenuText(menu, 0, yesno[cfg.msgShow != 0]);
        M_WriteMenuText(menu, 1, xhairnames[cfg.xhair]);
        MN_DrawSlider(menu, 3, 9, cfg.xhairSize);
        MN_DrawSlider(menu, 6, 11, cfg.screenBlocks - 3);
        MN_DrawSlider(menu, 9, 20, cfg.statusbarScale - 1);
        MN_DrawSlider(menu, 12, 11, cfg.statusbarAlpha * 10 + .25f);
    }
    else
    {
        M_WriteMenuText(menu, 16, yesno[cfg.hudShown[HUD_MANA] != 0]);
        M_WriteMenuText(menu, 17, yesno[cfg.hudShown[HUD_HEALTH]]);
        M_WriteMenuText(menu, 18, yesno[cfg.hudShown[HUD_ARTI]]);
        MN_DrawColorBox(menu,19, cfg.hudColor[0], cfg.hudColor[1],
                        cfg.hudColor[2], menuAlpha);
        MN_DrawSlider(menu, 21, 10, cfg.hudScale * 10 - 3 + .5f);
    }
#elif __JHERETIC__
    if(menu->firstItem < menu->numVisItems)
    {
        M_WriteMenuText(menu, 0, yesno[cfg.msgShow != 0]);
        M_WriteMenuText(menu, 1, xhairnames[cfg.xhair]);
        MN_DrawSlider(menu, 3, 9, cfg.xhairSize);
        MN_DrawSlider(menu, 6, 11, cfg.screenBlocks - 3);
        MN_DrawSlider(menu, 9, 20, cfg.statusbarScale - 1);
        MN_DrawSlider(menu, 12, 11, cfg.statusbarAlpha * 10 + .25f);
    }
    else
    {
        M_WriteMenuText(menu, 16, yesno[cfg.hudShown[HUD_AMMO]]);
        M_WriteMenuText(menu, 17, yesno[cfg.hudShown[HUD_ARMOR]]);
        M_WriteMenuText(menu, 18, yesno[cfg.hudShown[HUD_ARTI]]);
        M_WriteMenuText(menu, 19, yesno[cfg.hudShown[HUD_HEALTH]]);
        M_WriteMenuText(menu, 20, yesno[cfg.hudShown[HUD_KEYS]]);
        MN_DrawColorBox(menu, 21, cfg.hudColor[0], cfg.hudColor[1],
                        cfg.hudColor[2], menuAlpha);
        MN_DrawSlider(menu, 23, 10, cfg.hudScale * 10 - 3 + .5f);
    }
#elif __JDOOM__ || __JDOOM64__
    idx = 0;
    M_WriteMenuText(menu, idx++, yesno[cfg.hudShown[HUD_AMMO]]);
    M_WriteMenuText(menu, idx++, yesno[cfg.hudShown[HUD_ARMOR]]);
# if __JDOOM64__
    M_WriteMenuText(menu, idx++, yesno[cfg.hudShown[HUD_POWER]]);
# else
    M_WriteMenuText(menu, idx++, yesno[cfg.hudShown[HUD_FACE]]);
# endif
    M_WriteMenuText(menu, idx++, yesno[cfg.hudShown[HUD_HEALTH]]);
    M_WriteMenuText(menu, idx++, yesno[cfg.hudShown[HUD_KEYS]]);
# if __JDOOM__
    M_WriteMenuText(menu, idx++, yesno[cfg.hudKeysCombine]);
# endif
    MN_DrawSlider(menu, idx++, 10, cfg.hudScale * 10 - 3 + .5f);
    MN_DrawColorBox(menu, idx++, cfg.hudColor[0], cfg.hudColor[1],
                    cfg.hudColor[2], menuAlpha);
    M_WriteMenuText(menu, idx++, yesno[cfg.msgShow != 0]);
    M_WriteMenuText(menu, idx++, xhairnames[cfg.xhair]);
    MN_DrawSlider(menu, idx++, 9, cfg.xhairSize);
    MN_DrawSlider(menu, idx++, 11, cfg.screenBlocks - 3 );
# if !__JDOOM64__
    MN_DrawSlider(menu, idx++, 20, cfg.statusbarScale - 1);
    MN_DrawSlider(menu, idx++, 11, cfg.statusbarAlpha * 10 + .25f);
# endif
#endif
}

void M_FloatMod10(float *variable, int option)
{
    int         val = (*variable + .05f) * 10;

    if(option == RIGHT_DIR)
    {
        if(val < 10)
            val++;
    }
    else if(val > 0)
        val--;
    *variable = val / 10.0f;
}

void M_Xhair(int option, void* context)
{
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    cfg.xhair += option == RIGHT_DIR ? 1 : -1;
    if(cfg.xhair < 0)
        cfg.xhair = 0;
    if(cfg.xhair > NUM_XHAIRS)
        cfg.xhair = NUM_XHAIRS;
#else
    if(option == RIGHT_DIR)
    {
        if(cfg.xhair < NUM_XHAIRS)
            cfg.xhair++;
    }
    else if(cfg.xhair > 0)
        cfg.xhair--;
#endif
}

void M_XhairSize(int option, void* context)
{
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    cfg.xhairSize += option == RIGHT_DIR ? 1 : -1;
    if(cfg.xhairSize < 0)
        cfg.xhairSize = 0;
    if(cfg.xhairSize > 9)
        cfg.xhairSize = 9;
#else
    if(option == RIGHT_DIR)
    {
        if(cfg.xhairSize < 8)
            cfg.xhairSize++;
    }
    else if(cfg.xhairSize > 0)
        cfg.xhairSize--;
#endif
}


#if __JDOOM__ || __JDOOM64__
void M_XhairR(int option, void* context)
{
    int         val = cfg.xhairColor[0];

    val += (option == RIGHT_DIR ? 17 : -17);
    if(val < 0)
        val = 0;
    if(val > 255)
        val = 255;
    cfg.xhairColor[0] = val;
}

void M_XhairG(int option, void* context)
{
    int         val = cfg.xhairColor[1];

    val += (option == RIGHT_DIR ? 17 : -17);
    if(val < 0)
        val = 0;
    if(val > 255)
        val = 255;
    cfg.xhairColor[1] = val;
}

void M_XhairB(int option, void* context)
{
    int         val = cfg.xhairColor[2];

    val += (option == RIGHT_DIR ? 17 : -17);
    if(val < 0)
        val = 0;
    if(val > 255)
        val = 255;
    cfg.xhairColor[2] = val;
}

void M_XhairAlpha(int option, void* context)
{
    int         val = cfg.xhairColor[3];

    val += (option == RIGHT_DIR ? 17 : -17);
    if(val < 0)
        val = 0;
    if(val > 255)
        val = 255;
    cfg.xhairColor[3] = val;
}
#endif

#if __JDOOM64__
void M_WeaponRecoil(int option, void* context)
{
    cfg.weaponRecoil = !cfg.weaponRecoil;
}
#endif

#if !__JDOOM64__
void M_SizeStatusBar(int option, void* context)
{
    if(option == RIGHT_DIR)
    {
        if(cfg.statusbarScale < 20)
            cfg.statusbarScale++;
    }
    else if(cfg.statusbarScale > 1)
        cfg.statusbarScale--;

    ST_HUDUnHide(CONSOLEPLAYER, HUE_FORCE);

    R_SetViewSize(cfg.screenBlocks);
}

void M_StatusBarAlpha(int option, void* context)
{
    M_FloatMod10(&cfg.statusbarAlpha, option);

    ST_HUDUnHide(CONSOLEPLAYER, HUE_FORCE);
}
#endif

void M_WGCurrentColor(int option, void* context)
{
    M_FloatMod10(context, option);
}

void M_NewGame(int option, void* context)
{
    if(IS_NETGAME)
    {
        Hu_MsgStart(MSG_ANYKEY, NEWGAME, NULL, NULL);
        return;
    }

#if __JHEXEN__
    M_SetupNextMenu(&ClassDef);
#elif __JHERETIC__
    M_SetupNextMenu(&EpiDef);
#elif __JDOOM64__ || __JSTRIFE__
    M_SetupNextMenu(&SkillDef);
#else // __JDOOM__
    if(gameMode == commercial)
        M_SetupNextMenu(&SkillDef);
    else
        M_SetupNextMenu(&EpiDef);
#endif
}

int M_QuitResponse(msgresponse_t response, void* context)
{
#if __JDOOM__ || __JDOOM64__
    static int quitsounds[8] = {
        SFX_PLDETH,
        SFX_DMPAIN,
        SFX_POPAIN,
        SFX_SLOP,
        SFX_TELEPT,
        SFX_POSIT1,
        SFX_POSIT3,
        SFX_SGTATK
    };
    static int quitsounds2[8] = {
        SFX_VILACT,
        SFX_GETPOW,
# if __JDOOM64__
        SFX_PEPAIN,
# else
        SFX_BOSCUB,
# endif
        SFX_SLOP,
        SFX_SKESWG,
        SFX_KNTDTH,
        SFX_BSPACT,
        SFX_SGTATK
    };
#endif

    if(response == MSG_YES)
    {
        Hu_MenuCommand(MCMD_CLOSEFAST);

#if __JDOOM__ || __JDOOM64__
        // Play an exit sound if it is enabled.
        if(cfg.menuQuitSound && !IS_NETGAME)
        {
            if(!quitYet)
            {
                if(gameMode == commercial)
                    S_LocalSound(quitsounds2[((int)GAMETIC >> 2) & 7], NULL);
                else
                    S_LocalSound(quitsounds[((int)GAMETIC >> 2) & 7], NULL);

                // Wait for 1.5 seconds.
                DD_Executef(true, "after 53 quit!");
                quitYet = true;
            }
        }
        else
        {
            Sys_Quit();
        }
#else
        Sys_Quit();
#endif
    }

    return true;
}

void M_QuitDOOM(int option, void* context)
{
    const char*         endString;

#if __JDOOM__ || __JDOOM64__
    endString = endmsg[((int) GAMETIC % (NUM_QUITMESSAGES + 1))];
#else
    endString = GET_TXT(TXT_QUITMSG);
#endif

    Con_Open(false);
    Hu_MsgStart(MSG_YESNO, endString, M_QuitResponse, NULL);
}

int M_EndGameResponse(msgresponse_t response, void* context)
{
    if(response == MSG_YES)
    {
        G_StartTitle();
        return true;
    }

    return true;
}

void M_EndGame(int option, void* context)
{
    if(!userGame)
    {
        Hu_MsgStart(MSG_ANYKEY, ENDNOGAME, NULL, NULL);
        return;
    }

    if(IS_NETGAME)
    {
        Hu_MsgStart(MSG_ANYKEY, NETEND, NULL, NULL);
        return;
    }

    Hu_MsgStart(MSG_YESNO, ENDGAME, M_EndGameResponse, NULL);
}

void M_ChangeMessages(int option, void* context)
{
    cfg.msgShow = !cfg.msgShow;
    P_SetMessage(players + CONSOLEPLAYER, !cfg.msgShow ? MSGOFF : MSGON, true);
}

void M_HUDScale(int option, void* context)
{
    int                 val = (cfg.hudScale + .05f) * 10;

    if(option == RIGHT_DIR)
    {
        if(val < 12)
            val++;
    }
    else if(val > 3)
        val--;

    cfg.hudScale = val / 10.0f;
    ST_HUDUnHide(CONSOLEPLAYER, HUE_FORCE);
}

#if __JDOOM__ || __JDOOM64__
void M_HUDRed(int option, void* context)
{
    M_FloatMod10(&cfg.hudColor[0], option);
}

void M_HUDGreen(int option, void* context)
{
    M_FloatMod10(&cfg.hudColor[1], option);
}

void M_HUDBlue(int option, void* context)
{
    M_FloatMod10(&cfg.hudColor[2], option);
}
#endif

void M_LoadGame(int option, void* context)
{
    if(IS_CLIENT && !Get(DD_PLAYBACK))
    {
        Hu_MsgStart(MSG_ANYKEY, LOADNET, NULL, NULL);
        return;
    }

    updateSaveList();
    M_SetupNextMenu(&LoadDef);
}

/**
 * Called via the menu or the control bindings mechanism when the player
 * wishes to save their game.
 */
void M_SaveGame(int option, void* context)
{
    player_t*           player = &players[CONSOLEPLAYER];

    if(player->playerState == PST_DEAD || Get(DD_PLAYBACK))
    {
        Hu_MsgStart(MSG_ANYKEY, SAVEDEAD, NULL, NULL);
        return;
    }

    if(G_GetGameState() != GS_MAP)
    {
        Hu_MsgStart(MSG_ANYKEY, SAVEOUTMAP, NULL, NULL);
        return;
    }

    if(IS_CLIENT)
    {
#if __JDOOM__ || __JDOOM64__
        Hu_MsgStart(MSG_ANYKEY, SAVENET, NULL, NULL);
#endif
        return;
    }

    Hu_MenuCommand(MCMD_OPEN);
    updateSaveList();
    M_SetupNextMenu(&SaveDef);
}

void M_ChooseClass(int option, void* context)
{
#if __JHEXEN__
    if(IS_NETGAME)
    {
        P_SetMessage(&players[CONSOLEPLAYER],
                     "YOU CAN'T START A NEW GAME FROM WITHIN A NETGAME!", false);
        return;
    }

    if(option < 0)
    {   // Random class.
        // Number of user-selectable classes.
        MenuPClass = (menuTime / 5) % (ClassDef.itemCount - 1);
    }
    else
    {
        MenuPClass = option;
    }

    switch(MenuPClass)
    {
    case PCLASS_FIGHTER:
        SkillDef.x = 120;
        SkillItems[0].text = GET_TXT(TXT_SKILLF1);
        SkillItems[1].text = GET_TXT(TXT_SKILLF2);
        SkillItems[2].text = GET_TXT(TXT_SKILLF3);
        SkillItems[3].text = GET_TXT(TXT_SKILLF4);
        SkillItems[4].text = GET_TXT(TXT_SKILLF5);
        break;

    case PCLASS_CLERIC:
        SkillDef.x = 116;
        SkillItems[0].text = GET_TXT(TXT_SKILLC1);
        SkillItems[1].text = GET_TXT(TXT_SKILLC2);
        SkillItems[2].text = GET_TXT(TXT_SKILLC3);
        SkillItems[3].text = GET_TXT(TXT_SKILLC4);
        SkillItems[4].text = GET_TXT(TXT_SKILLC5);
        break;

    case PCLASS_MAGE:
        SkillDef.x = 112;
        SkillItems[0].text = GET_TXT(TXT_SKILLM1);
        SkillItems[1].text = GET_TXT(TXT_SKILLM2);
        SkillItems[2].text = GET_TXT(TXT_SKILLM3);
        SkillItems[3].text = GET_TXT(TXT_SKILLM4);
        SkillItems[4].text = GET_TXT(TXT_SKILLM5);
        break;
    }
    M_SetupNextMenu(&SkillDef);
#endif
}

#if __JDOOM__ || __JHERETIC__
void M_Episode(int option, void* context)
{
#if __JHERETIC__
    if(shareware && option > 1)
    {
        Hu_MsgStart(MSG_ANYKEY, SWSTRING, NULL, NULL);
        M_SetupNextMenu(&ReadDef1);
        return;
    }
#else
    if(gameMode == shareware && option)
    {
        Hu_MsgStart(MSG_ANYKEY, SWSTRING, NULL, NULL);
        M_SetupNextMenu(&ReadDef1);
        return;
    }
#endif

    epi = option;
    M_SetupNextMenu(&SkillDef);
}
#endif

#if __JDOOM__ || __JHERETIC__
int M_VerifyNightmare(msgresponse_t response, void* context)
{
    if(response == MSG_YES)
    {
        Hu_MenuCommand(MCMD_CLOSEFAST);
        G_DeferedInitNew(SM_NIGHTMARE, epi + 1, 1);
    }

    return true;
}
#endif

void M_ChooseSkill(int option, void* context)
{
#if __JHEXEN__
    Hu_MenuCommand(MCMD_CLOSEFAST);
    cfg.playerClass[CONSOLEPLAYER] = MenuPClass;
    G_DeferredNewGame(option);
#else
# if __JDOOM__ || __JSTRIFE__
    if(option == SM_NIGHTMARE)
    {
        Hu_MsgStart(MSG_YESNO, NIGHTMARE, M_VerifyNightmare, NULL);
        return;
    }
# endif

    Hu_MenuCommand(MCMD_CLOSEFAST);

# if __JDOOM64__
    G_DeferedInitNew(option, 1, 1);
# else
    G_DeferedInitNew(option, epi + 1, 1);
# endif
#endif
}

void M_SfxVol(int option, void* context)
{
    int                 vol = SFXVOLUME;

    if(option == RIGHT_DIR)
    {
        if(vol < 15)
            vol++;
    }
    else
    {
        if(vol > 0)
            vol--;
    }

    Set(DD_SFX_VOLUME, vol * 17);
}

void M_MusicVol(int option, void* context)
{
    int                 vol = MUSICVOLUME;

    if(option == RIGHT_DIR)
    {
        if(vol < 15)
            vol++;
    }
    else
    {
        if(vol > 0)
            vol--;
    }

    Set(DD_MUSIC_VOLUME, vol * 17);
}

void M_SizeDisplay(int option, void* context)
{
    if(option == RIGHT_DIR)
    {
#if __JDOOM64__
        if(cfg.screenBlocks < 11)
#else
        if(cfg.screenBlocks < 13)
#endif
        {
            cfg.screenBlocks++;
        }
    }
    else if(cfg.screenBlocks > 3)
    {
        cfg.screenBlocks--;
    }

    R_SetViewSize(cfg.screenBlocks);
}

void M_OpenDCP(int option, void* context)
{
#define NUM_PANEL_NAMES 3
    static const char *panelNames[] = {
        "panel",
        "panel audio",
        "panel input"
    };
    int                 idx = option;

    if(idx < 0 || idx > NUM_PANEL_NAMES - 1)
        idx = 0;

    Hu_MenuCommand(MCMD_CLOSEFAST);
    DD_Execute(true, panelNames[idx]);

#undef NUM_PANEL_NAMES
}

void MN_DrawColorBox(const menu_t *menu, int index, float r, float g,
                     float b, float a)
{
    int                 x = menu->x + 4;
    int                 y =
        menu->y + menu->itemHeight * (index - menu->firstItem) + 3;

    M_DrawColorBox(x, y, r, g, b, a);
}

/**
 * Draws a menu slider control
 */
void MN_DrawSlider(const menu_t* menu, int item, int width, int slot)
{
#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    int                 x;
    int                 y;

    x = menu->x + 24;
    y = menu->y + 2 + (menu->itemHeight * (item  - menu->firstItem));

    M_DrawSlider(x, y, width, slot, menuAlpha);
#else
    int                 x =0, y =0;
    int                 height = menu->itemHeight - 1;
    float               scale = height / 13.0f;

    if(menu->items[item].text)
        x = M_StringWidth(menu->items[item].text, menu->font);

    x += menu->x + 6;
    y = menu->y + menu->itemHeight * item;

    M_DrawSlider(x, y, width, height, slot, menuAlpha);
#endif
}

/**
 * Routes menu commands, actions and navigation.
 */
DEFCC(CCmdMenuAction)
{
    int                 mode = 0;

    if(!menuActive)
    {
        if(!stricmp(argv[0], "menu") && !chatOn) // Open menu.
        {
            Hu_MenuCommand(MCMD_OPEN);
            return true;
        }
    }
    else
    {
        // Determine what state the menu is in currently
        if(ActiveEdit)
            mode = 1;
        else if(widgetEdit)
            mode = 2;
        else if(saveStringEnter)
            mode = 3;
#if !__JDOOM64__
        else
        {
            if(currentMenu == &ReadDef1 || currentMenu == &ReadDef2
# if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
               || currentMenu == &ReadDef3
# endif
               )
                mode = 4;
        }
#endif

        if(!stricmp(argv[0], "menuup"))
        {
            switch(mode)
            {
            case 2: // Widget edit
                if(!widgetEdit)
                    break;
                // Fall through.

            case 0: // Menu nav
                Hu_MenuCommand(MCMD_NAV_UP);
                break;

            default:
                break;
            }
            return true;
        }
        else if(!stricmp(argv[0], "menudown"))
        {
            switch(mode)
            {
            case 2: // Widget edit
                if(!widgetEdit)
                    break;
                // Fall through.

            case 0: // Menu nav
                Hu_MenuCommand(MCMD_NAV_DOWN);
                break;

            default:
                break;
            }
            return true;
        }
        else if(!stricmp(argv[0], "menuleft"))
        {
            switch(mode)
            {
            case 0: // Menu nav
            case 2: // Widget edit
                Hu_MenuCommand(MCMD_NAV_LEFT);
                break;

            default:
                break;
            }
            return true;
        }
        else if(!stricmp(argv[0], "menuright"))
        {
            switch(mode)
            {
            case 0: // Menu nav
            case 2: // Widget edit
                Hu_MenuCommand(MCMD_NAV_RIGHT);
                break;

            default:
                break;
            }
            return true;
        }
        else if(!stricmp(argv[0], "menudelete"))
        {
            if(!mode)
            {
                Hu_MenuCommand(MCMD_DELETE);
            }
            return true;
        }
        else if(!stricmp(argv[0], "menuselect"))
        {
            switch(mode)
            {
            case 4: // In helpscreens
            case 0: // Menu nav
                Hu_MenuCommand(MCMD_SELECT);
                break;

            case 1: // Edit Field
                ActiveEdit->firstVisible = 0;
                ActiveEdit = NULL;
                S_LocalSound(menusnds[0], NULL);
                break;

            case 2: // Widget edit
                // Set the new color
                *widgetcolors[editcolorindex].r = currentcolor[0];
                *widgetcolors[editcolorindex].g = currentcolor[1];
                *widgetcolors[editcolorindex].b = currentcolor[2];

                if(rgba)
                    *widgetcolors[editcolorindex].a = currentcolor[3];

                // Restore the position of the skull
                itemOn = previtemOn;

                widgetEdit = false;
                S_LocalSound(menusnds[5], NULL);
                break;

            case 3: // Save string edit: Save
                saveStringEnter = 0;
                if(savegamestrings[saveSlot][0])
                    M_DoSave(saveSlot);
                break;
            }
            return true;
        }
        else if(!stricmp(argv[0], "menuback"))
        {
            int         c;

            switch(mode)
            {
            case 0: // Menu nav: Previous menu
                Hu_MenuCommand(MCMD_NAV_OUT);
                break;

            case 1: // Edit Field: Del char
                c = strlen(ActiveEdit->text);
                if(c > 0)
                    ActiveEdit->text[c - 1] = 0;
                Ed_MakeCursorVisible();
                break;

            case 2: // Widget edit: Close widget
                // Restore the position of the skull
                itemOn = previtemOn;
                widgetEdit = false;
                S_LocalSound(menusnds[1], NULL);
                break;

            case 3: // Save string edit: Del char
                if(saveCharIndex > 0)
                {
                    saveCharIndex--;
                    savegamestrings[saveSlot][saveCharIndex] = 0;
                }
                break;
            }

            return true;
        }
        else if(!stricmp(argv[0], "menu"))
        {
            switch(mode)
            {
            case 0: // Menu nav: Close menu
                currentMenu->lastOn = itemOn;
                Hu_MenuCommand(MCMD_CLOSE);
                S_LocalSound(menusnds[1], NULL);
                break;

            case 1: // Edit Field
                ActiveEdit->firstVisible = 0;
                strcpy(ActiveEdit->text, ActiveEdit->oldtext);
                ActiveEdit = NULL;
                break;

            case 2: // Widget edit: Close widget
                // Restore the position of the skull
                itemOn = previtemOn;
                widgetEdit = false;
                S_LocalSound(menusnds[1], NULL);
                break;

            case 3: // Save string edit: Cancel
                saveStringEnter = 0;
                strcpy(&savegamestrings[saveSlot][0], saveOldString);
                break;

            case 4: // In helpscreens: Exit and close menu
                M_SetupNextMenu(&MainDef);
                Hu_MenuCommand(MCMD_CLOSEFAST);
                break;
            }

            return true;
        }
    }

    // Hotkey shortcuts.
#if !__JDOOM64__
    if(!stricmp(argv[0], "helpscreen"))
    {
        Hu_MenuCommand(MCMD_OPEN);
        menuTime = 0;
# if __JDOOM__
        if(gameMode == retail)
            currentMenu = &ReadDef2;
        else
# endif
            currentMenu = &ReadDef1;

        itemOn = 0;
        //S_LocalSound(menusnds[2], NULL);
    }
    else
#endif
        if(!stricmp(argv[0], "SaveGame"))
    {
        menuTime = 0;
        //S_LocalSound(menusnds[2], NULL);
        M_SaveGame(0, NULL);
    }
    else if(!stricmp(argv[0], "LoadGame"))
    {
        Hu_MenuCommand(MCMD_OPEN);
        menuTime = 0;
        //S_LocalSound(menusnds[2], NULL);
        M_LoadGame(0, NULL);
    }
    else if(!stricmp(argv[0], "SoundMenu"))
    {
        Hu_MenuCommand(MCMD_OPEN);
        menuTime = 0;
        currentMenu = &Options2Def;
        itemOn = 0;
        //S_LocalSound(menusnds[2], NULL);
    }
    else if(!stricmp(argv[0], "QuickSave"))
    {
        S_LocalSound(menusnds[2], NULL);
        menuTime = 0;
        M_QuickSave();
    }
    else if(!stricmp(argv[0], "EndGame"))
    {
        S_LocalSound(menusnds[2], NULL);
        menuTime = 0;
        M_EndGame(0, NULL);
    }
    else if(!stricmp(argv[0], "ToggleMsgs"))
    {
        menuTime = 0;
        M_ChangeMessages(0, NULL);
        S_LocalSound(menusnds[2], NULL);
    }
    else if(!stricmp(argv[0], "QuickLoad"))
    {
        S_LocalSound(menusnds[2], NULL);
        menuTime = 0;
        M_QuickLoad();
    }
    else if(!stricmp(argv[0], "quit"))
    {
        if(IS_DEDICATED)
            DD_Execute(true, "quit!");
        else
        {
            S_LocalSound(menusnds[2], NULL);
            menuTime = 0;
            M_QuitDOOM(0, NULL);
        }
    }
    else if(!stricmp(argv[0], "ToggleGamma"))
    {
        R_CycleGammaLevel();
    }

    return true;
}
