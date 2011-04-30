/**\file hu_log.c
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2005-2011 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2005-2011 Daniel Swanson <danij@dengine.net>
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#if __JDOOM__
#  include "jdoom.h"
#elif __JDOOM64__
#  include "jdoom64.h"
#elif __JHERETIC__
#  include "jheretic.h"
#elif __JHEXEN__
#  include "jhexen.h"
#endif

#include "hu_log.h"
#include "hu_stuff.h"
#include "p_tick.h" // for P_IsPaused()
#include "d_net.h"

#define LOG_MAX_MESSAGES    (8)

#define LOG_MSG_FLASHFADETICS (1*TICSPERSEC)
#define LOG_MSG_TIMEOUT     (4*TICRATE)

/**
 * @defgroup messageFlags  Message Flags.
 * @{
 */
#define MF_JUSTADDED        0x1 
#define MF_NOHIDE           0x2 /// Message cannot be hidden.
/**@}*/

typedef struct logmsg_s {
    char* text;
    size_t maxLen;
    uint ticsRemain, tics;
    int flags;
} logmsg_t;

typedef struct msglog_s {
    /// @c true if this log is presently visible.
    boolean visible;

    /// Log message list.
    logmsg_t msgs[LOG_MAX_MESSAGES];

    /// Number of used msg slots.
    uint msgCount;

    /// Index of the next slot to be used in msgs.
    uint nextMsg;

    /// Number of potentially-visible messages.
    uint numVisibleMsgs;

    /// Auto-hide timer.
    int timer;
} msglog_t;

D_CMD(LocalMessage);

void Hu_LogPostVisibilityChangeNotification(void);

static msglog_t msgLogs[MAXPLAYERS];

cvartemplate_t msgLogCVars[] = {
    // Behaviour
    { "msg-count",  0, CVT_INT, &cfg.msgCount, 1, 8 },
    { "msg-uptime", 0, CVT_FLOAT, &cfg.msgUptime, 1, 60 },

    // Display
    { "msg-align", 0, CVT_INT, &cfg.msgAlign, 0, 2 },
    { "msg-blink", CVF_NO_MAX, CVT_INT, &cfg.msgBlink, 0, 0 },
    { "msg-scale", 0, CVT_FLOAT, &cfg.msgScale, 0.1f, 1 },
    { "msg-show",  0, CVT_BYTE, &cfg.hudShown[HUD_LOG], 0, 1, Hu_LogPostVisibilityChangeNotification },

    // Colour defaults
    { "msg-color-r", 0, CVT_FLOAT, &cfg.msgColor[CR], 0, 1 },
    { "msg-color-g", 0, CVT_FLOAT, &cfg.msgColor[CG], 0, 1 },
    { "msg-color-b", 0, CVT_FLOAT, &cfg.msgColor[CB], 0, 1 },
    { NULL }
};

ccmdtemplate_t msgLogCCmds[] = {
    { "message", "s", CCmdLocalMessage },
    { NULL }
};

void Hu_LogRegister(void)
{
    int i;
    for(i = 0; msgLogCVars[i].name; ++i)
        Con_AddVariable(msgLogCVars + i);
    for(i = 0; msgLogCCmds[i].name; ++i)
        Con_AddCommand(msgLogCCmds + i);
}

/**
 * Push a new message into to log.
 *
 * @param flags  @see messageFlags
 * @param txt  The message to be added.
 * @param tics  The length of time the message should be visible.
 */
static void logPush(msglog_t* log, int flags, const char* txt, int tics)
{
    size_t len;
    logmsg_t* msg;

    if(!txt || !txt[0])
        return;

    len = strlen(txt);
    msg = &log->msgs[log->nextMsg];

    if(len >= msg->maxLen)
    {
        msg->maxLen = len+1;
        msg->text = realloc(msg->text, msg->maxLen);
    }

    memset(msg->text, 0, msg->maxLen);
    dd_snprintf(msg->text, msg->maxLen, "%s", txt);
    msg->ticsRemain = msg->tics = tics;
    msg->flags = MF_JUSTADDED | flags;

    if(log->nextMsg < LOG_MAX_MESSAGES - 1)
        log->nextMsg++;
    else
        log->nextMsg = 0;

    if(log->msgCount < LOG_MAX_MESSAGES)
        log->msgCount++;

    if(log->numVisibleMsgs < (unsigned) cfg.msgCount)
        log->numVisibleMsgs++;

    // Reset the auto-hide timer.
    log->timer = LOG_MSG_TIMEOUT;

    log->visible = true;
}

/**
 * Remove the oldest message from the log.
 */
static void logPop(msglog_t* log)
{
    int oldest;
    logmsg_t* msg;

    if(log->numVisibleMsgs == 0)
        return;

    oldest = (unsigned) log->nextMsg - log->numVisibleMsgs;
    if(oldest < 0)
        oldest += LOG_MAX_MESSAGES;

    msg = &log->msgs[oldest];
    msg->ticsRemain = 10;
    msg->flags &= ~MF_JUSTADDED;

    log->numVisibleMsgs--;
}

/**
 * Process gametic. Jobs include ticking messages and adjusting values
 * used when drawing the buffer for animation.
 */
static void logTicker(msglog_t* log)
{
    // Don't tick if the game is paused.
    if(P_IsPaused())
        return;

    // All messags tic away. When zero, the earliest is pop'd.
    { int i;
    for(i = 0; i < LOG_MAX_MESSAGES; ++i)
    {
        logmsg_t* msg = &log->msgs[i];

        if(msg->ticsRemain > 0)
            msg->ticsRemain--;
    }}

    if(log->numVisibleMsgs)
    {
        int oldest;
        logmsg_t* msg;

        oldest = (unsigned) log->nextMsg - log->numVisibleMsgs;
        if(oldest < 0)
            oldest += LOG_MAX_MESSAGES;

        msg = &log->msgs[oldest];

        if(msg->ticsRemain == 0)
        {
            logPop(log);
        }
    }

    // Tic the auto-hide timer.
    if(log->timer > 0)
    {
        --log->timer;
    }
    if(log->timer == 0)
    {
        log->visible = false;
    }
}

void Hu_LogStart(int player)
{
    player_t* plr;
    msglog_t* log;

    if(player < 0 || player >= MAXPLAYERS)
        return;

    plr = &players[player];
    if(!((plr->plr->flags & DDPF_LOCAL) && plr->plr->inGame))
        return;

    log = &msgLogs[player];
    memset(log, 0, sizeof(msglog_t));
}

void Hu_LogShutdown(void)
{
    int i;
    for(i = 0; i < MAXPLAYERS; ++i)
    {
        msglog_t* log = &msgLogs[i];
        int j;

        for(j = 0; j < LOG_MAX_MESSAGES; ++j)
        {
            logmsg_t* msg = &log->msgs[j];
            if(msg->text)
                free(msg->text);
            msg->text = NULL;
            msg->maxLen = 0;
        }

        log->msgCount = log->numVisibleMsgs = 0;
    }
}

void Hu_LogTicker(void)
{
    int i;
    for(i = 0; i < MAXPLAYERS; ++i)
    {
        logTicker(&msgLogs[i]);
    }
}

static __inline uint calcPVisMessageCount(msglog_t* log)
{
    assert(NULL != log);
    return MIN_OF(log->numVisibleMsgs, (unsigned) cfg.msgCount);
}

void Hu_LogDrawer(int player, float textAlpha, float iconAlpha,
    int* drawnWidth, int* drawnHeight)
{
    assert(player >= 0 && player < MAXPLAYERS);
    {
    const short textFlags = DTF_ALIGN_TOP|DTF_NO_EFFECTS | ((cfg.msgAlign == 0)? DTF_ALIGN_LEFT : (cfg.msgAlign == 2)? DTF_ALIGN_RIGHT : 0);
    msglog_t* log = &msgLogs[player];
    int n, y, viewW, viewH, lineHeight, firstPVisMsg, firstMsg;
    uint i, pvisMsgCount;
    float scale, yOffset;
    logmsg_t* msg;

    *drawnWidth  = 0;
    *drawnHeight = 0;

    /// \kludge Do not draw the message log while the map title is being displayed.
    if(cfg.mapTitle && actualMapTime < 6 * 35)
        return;
    /// kludge end.

    pvisMsgCount = calcPVisMessageCount(log);
    if(0 == pvisMsgCount) return;

    // Determine the index of the first potentially-visible message.
    firstPVisMsg = (unsigned)log->nextMsg - (unsigned)pvisMsgCount;
    if(firstPVisMsg < 0)
        firstPVisMsg += LOG_MAX_MESSAGES; // Wrap around.

    firstMsg = firstPVisMsg;
    if(!cfg.hudShown[HUD_LOG])
    {
        // Advance to the first non-hidden message.
        i = 0;
        while(0 == (log->msgs[firstMsg].flags & MF_NOHIDE) && ++i < pvisMsgCount)
        {
            ++firstMsg;
        }

        // Nothing visible?
        if(i == pvisMsgCount) return;

        // There is possibly fewer potentially-visible messages now.
        pvisMsgCount -= firstMsg - firstPVisMsg;
    } 

    FR_SetFont(FID(GF_FONTA));
    /// \fixme Query line height from the font.
    lineHeight = FR_CharHeight('Q')+1;

    R_GetViewPort(player, NULL, NULL, &viewW, &viewH);
    scale = viewW >= viewH? (float)viewH/SCREENHEIGHT : (float)viewW/SCREENWIDTH;

    // Scroll offset is calculated using the timeout of the first visible message.
    msg = &log->msgs[firstMsg];
    if(msg->ticsRemain > 0 && msg->ticsRemain <= (unsigned) lineHeight)
        yOffset = -(lineHeight-2) * (1.f - ((float)(msg->ticsRemain)/lineHeight));
    else
        yOffset = 0;
    yOffset *= scale;

    DGL_MatrixMode(DGL_PROJECTION);
    DGL_Translatef(0, yOffset, 0);
    DGL_Enable(DGL_TEXTURE_2D);

    y = 0;
    n = firstMsg;
    for(i = 0; i < pvisMsgCount; ++i, n = (n < LOG_MAX_MESSAGES - 1)? n + 1 : 0)
    {
        int width, height;
        float col[4];

        msg = &log->msgs[n];
        if(!cfg.hudShown[HUD_LOG] && !(msg->flags & MF_NOHIDE))
            continue;

        FR_TextDimensions(&width, &height, msg->text, FID(GF_FONTA));

        // Default color and alpha.
        col[CR] = cfg.msgColor[CR];
        col[CG] = cfg.msgColor[CG];
        col[CB] = cfg.msgColor[CB];
        col[CA] = textAlpha;

        if(msg->flags & MF_JUSTADDED)
        {
            uint msgTics, td, blinkSpeed = cfg.msgBlink;

            msgTics = msg->tics - msg->ticsRemain;
            td = (cfg.msgUptime * TICSPERSEC) - msg->ticsRemain;

            if((td & 2) && blinkSpeed != 0 && msgTics < blinkSpeed)
            {
                // Flash color.
                col[CR] = col[CG] = col[CB] = 1;
            }
            else if(blinkSpeed != 0 && msgTics < blinkSpeed + LOG_MSG_FLASHFADETICS && msgTics >= blinkSpeed)
            {
                // Fade color to normal.
                float fade = (blinkSpeed + LOG_MSG_FLASHFADETICS - msgTics);
                int c;
                for(c = 0; c < 3; ++c)
                    col[c] += (1.0f - col[c]) / LOG_MSG_FLASHFADETICS * fade;
            }
        }
        else
        {
            // Fade alpha out.
            if(i == 0 && msg->ticsRemain <= (unsigned) height)
                col[CA] *= msg->ticsRemain / (float) height * .9f;
        }

        // Draw using param text.
        // Messages may use the params to override the way the message is
        // is displayed (e.g., coloring of Hexen's "important" messages).
        FR_DrawText(msg->text, 0, y, FID(GF_FONTA), textFlags, .5f, 0,
            col[CR], col[CG], col[CB], col[CA], 0, 0, false);

        if(width > *drawnWidth)
            *drawnWidth = width;
        *drawnHeight += height;

        y += height + 1;
    }

    DGL_Disable(DGL_TEXTURE_2D);

    DGL_MatrixMode(DGL_PROJECTION);
    DGL_Translatef(0, -yOffset, 0);

    *drawnHeight += (pvisMsgCount-1) * 1;
    *drawnHeight -= -yOffset/scale;
    }
}

void Hu_LogPost(int player, byte flags, const char* msg)
{
#define YELLOW_FMT      "{r=1; g=0.7; b=0.3;}"
#define YELLOW_FMT_LEN  19
#define SMALLBUF_MAXLEN 128

    char smallBuf[SMALLBUF_MAXLEN+1];
    char* bigBuf = NULL, *p;
    size_t requiredLen;
    player_t* plr;
    msglog_t* log;

    if(!msg || !msg[0])
        return;

    if(player < 0 || player >= MAXPLAYERS)
        return;

    plr = &players[player];
    if(!((plr->plr->flags & DDPF_LOCAL) && plr->plr->inGame))
        return;

    log = &msgLogs[player];

    requiredLen = strlen(msg) + ((flags & LMF_YELLOW)? YELLOW_FMT_LEN : 0);
    if(requiredLen <= SMALLBUF_MAXLEN)
    {
        p = smallBuf;
    }
    else
    {
        bigBuf = malloc(requiredLen + 1);
        p = bigBuf;
    }

    p[requiredLen] = '\0';
    if(flags & LMF_YELLOW)
        sprintf(p, YELLOW_FMT "%s", msg);
    else
        sprintf(p, "%s", msg);

    logPush(log, 0 | ((flags & LMF_NOHIDE) != 0? MF_NOHIDE : 0), p, cfg.msgUptime * TICSPERSEC);

    if(bigBuf)
        free(bigBuf);

#undef SMALLBUF_MAXLEN
#undef YELLOW_FMT_LEN
#undef YELLOW_FMT
}

void Hu_LogPostVisibilityChangeNotification(void)
{
    Hu_LogPost(CONSOLEPLAYER, LMF_NOHIDE, !cfg.hudShown[HUD_LOG] ? MSGOFF : MSGON);
}

void Hu_LogRefresh(int player)
{
    player_t* plr;
    msglog_t* log;
    uint i;
    int n;

    if(player < 0 || player >= MAXPLAYERS)
        return;

    plr = &players[player];
    if(!((plr->plr->flags & DDPF_LOCAL) && plr->plr->inGame))
        return;

    log = &msgLogs[player];
    log->visible = true;
    log->numVisibleMsgs = MIN_OF((unsigned) cfg.msgCount,
        MIN_OF(log->msgCount, (unsigned) LOG_MAX_MESSAGES));

    // Reset the auto-hide timer.
    log->timer = LOG_MSG_TIMEOUT;

    // Refresh the messages.
    n = log->nextMsg - log->numVisibleMsgs;
    if(n < 0)
        n += LOG_MAX_MESSAGES;

    for(i = 0; i < log->numVisibleMsgs; ++i)
    {
        logmsg_t* msg = &log->msgs[n];

        // Change the tics remaining to that at post time plus a small bonus
        // so that they don't all disappear at once.
        msg->ticsRemain = msg->tics + i * TICSPERSEC;
        msg->flags &= ~MF_JUSTADDED;

        n = (n < LOG_MAX_MESSAGES - 1)? n + 1 : 0;
    }
}

void Hu_LogEmpty(int player)
{
    player_t* plr;
    msglog_t* log;

    if(player < 0 || player >= MAXPLAYERS)
        return;

    plr = &players[player];
    if(!((plr->plr->flags & DDPF_LOCAL) && plr->plr->inGame))
        return;

    log = &msgLogs[player];

    while(log->numVisibleMsgs)
        logPop(log);
}

/**
 * Display a local game message.
 */
D_CMD(LocalMessage)
{
    D_NetMessageNoSound(CONSOLEPLAYER, argv[1]);
    return true;
}

/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2005-2009 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2005-2009 Daniel Swanson <danij@dengine.net>
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
 * hu_chat.c: HUD chat widget.
 */

// HEADER FILES ------------------------------------------------------------

#include <ctype.h>
#include <stdlib.h>
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
#endif

#include "hu_stuff.h"
#include "hu_log.h"
#include "hu_lib.h"
#include "p_tick.h" // for P_IsPaused()
#include "g_common.h"
#include "g_controls.h"
#include "d_net.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

#if __JHEXEN__
enum {
    CT_PLR_BLUE = 1,
    CT_PLR_RED,
    CT_PLR_YELLOW,
    CT_PLR_GREEN,
    CT_PLR_PLAYER5,
    CT_PLR_PLAYER6,
    CT_PLR_PLAYER7,
    CT_PLR_PLAYER8
};
#endif

typedef struct {
    boolean active;
    boolean shiftDown;
    int to; // 0=all, 1=player 0, etc.
    hu_text_t buffer;
} uiwidget_chat_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

D_CMD(MsgAction);

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void     closeChat(void);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

#if __JDOOM__ || __JHERETIC__ || __JDOOM64__

char* player_names[4];
int player_names_idx[] = {
    TXT_HUSTR_PLRGREEN,
    TXT_HUSTR_PLRINDIGO,
    TXT_HUSTR_PLRBROWN,
    TXT_HUSTR_PLRRED
};

#else

char* player_names[8];
int player_names_idx[] = {
    CT_PLR_BLUE,
    CT_PLR_RED,
    CT_PLR_YELLOW,
    CT_PLR_GREEN,
    CT_PLR_PLAYER5,
    CT_PLR_PLAYER6,
    CT_PLR_PLAYER7,
    CT_PLR_PLAYER8
};
#endif

cvartemplate_t chatCVars[] = {
    // Chat macros
    {"chat-macro0", 0, CVT_CHARPTR, &cfg.chatMacros[0], 0, 0},
    {"chat-macro1", 0, CVT_CHARPTR, &cfg.chatMacros[1], 0, 0},
    {"chat-macro2", 0, CVT_CHARPTR, &cfg.chatMacros[2], 0, 0},
    {"chat-macro3", 0, CVT_CHARPTR, &cfg.chatMacros[3], 0, 0},
    {"chat-macro4", 0, CVT_CHARPTR, &cfg.chatMacros[4], 0, 0},
    {"chat-macro5", 0, CVT_CHARPTR, &cfg.chatMacros[5], 0, 0},
    {"chat-macro6", 0, CVT_CHARPTR, &cfg.chatMacros[6], 0, 0},
    {"chat-macro7", 0, CVT_CHARPTR, &cfg.chatMacros[7], 0, 0},
    {"chat-macro8", 0, CVT_CHARPTR, &cfg.chatMacros[8], 0, 0},
    {"chat-macro9", 0, CVT_CHARPTR, &cfg.chatMacros[9], 0, 0},
    {"chat-beep", 0, CVT_BYTE, &cfg.chatBeep, 0, 1},
    {NULL}
};

// Console commands for the chat widget and message log.
ccmdtemplate_t chatCCmds[] = {
    {"chatcancel",      "",     CCmdMsgAction},
    {"chatcomplete",    "",     CCmdMsgAction},
    {"chatdelete",      "",     CCmdMsgAction},
    {"chatsendmacro",   NULL,   CCmdMsgAction},
    {"beginchat",       NULL,   CCmdMsgAction},
    {NULL}
};

// PRIVATE DATA DEFINITIONS ------------------------------------------------

uiwidget_chat_t chatWidgets[DDMAXPLAYERS];

// CODE --------------------------------------------------------------------

/**
 * Called during the PreInit of each game during start up.
 * Register Cvars and CCmds for the opperation/look of the (message) log.
 */
void Chat_Register(void)
{
    int i;

    for(i = 0; chatCVars[i].name; ++i)
        Con_AddVariable(chatCVars + i);

    for(i = 0; chatCCmds[i].name; ++i)
        Con_AddCommand(chatCCmds + i);
}

/**
 * Called by HU_init().
 */
void Chat_Init(void)
{
    int i;

    // Setup strings.
    for(i = 0; i < 10; ++i)
    {
        if(!cfg.chatMacros[i]) // Don't overwrite if already set.
            cfg.chatMacros[i] = GET_TXT(TXT_HUSTR_CHATMACRO0 + i);
    }
}

/**
 * Called by HU_Start().
 */
void Chat_Start(void)
{
    int i;

    // Create the chat widgets.
    for(i = 0; i < MAXPLAYERS; ++i)
    {
        Chat_Open(i, false);

        // Create the input buffers.
        HUlib_initText(&chatWidgets[i].buffer, 0, 0, &chatWidgets[i].active);
    }
}

void Chat_Open(int player, boolean open)
{
    assert(player >= 0 && player < DDMAXPLAYERS);
    {
    uiwidget_chat_t* chat = &chatWidgets[player];
    if(open)
    {
        chat->active = true;
        chat->to = player;

        HUlib_resetText(&chat->buffer);
        // Enable the chat binding class
        DD_Execute(true, "activatebcontext chat");
        return;
    }

    if(chat->active)
    {
        chat->active = false;
        // Disable the chat binding class
        DD_Execute(true, "deactivatebcontext chat");
    }
    }
}

boolean Chat_Responder(event_t* ev)
{
    int player = CONSOLEPLAYER;
    uiwidget_chat_t* chat = &chatWidgets[player];
    boolean eatkey = false;
    unsigned char c;

    if(!chat->active)
        return false;

    if(ev->type != EV_KEY)
        return false;

    if(ev->data1 == DDKEY_RSHIFT)
    {
        chat->shiftDown = (ev->state == EVS_DOWN || ev->state == EVS_REPEAT);
        return false;
    }

    if(ev->state != EVS_DOWN)
        return false;

    c = (unsigned char) ev->data1;

    if(chat->shiftDown)
        c = shiftXForm[c];

    eatkey = HUlib_keyInText(&chat->buffer, c);

    return eatkey;
}

void Chat_Drawer(int player, float textAlpha, float iconAlpha,
    int* drawnWidth, int* drawnHeight)
{
    assert(player >= 0 && player < DDMAXPLAYERS);
    {
    uiwidget_chat_t* chat = &chatWidgets[player];
    hu_textline_t* l = &chat->buffer.l;
    char buf[HU_MAXLINELENGTH+1];
    const char* str;
    int xOffset = 0;
    byte textFlags;

    if(!*chat->buffer.on)
        return;

    FR_SetFont(FID(GF_FONTA));
    if(actualMapTime & 12)
    {
        dd_snprintf(buf, HU_MAXLINELENGTH+1, "%s_", chat->buffer.l.l);
        str = buf;
        if(cfg.msgAlign == 1)
            xOffset = FR_CharWidth('_')/2;
    }
    else
    {
        str = chat->buffer.l.l;
        if(cfg.msgAlign == 2)
            xOffset = -FR_CharWidth('_');
    }
    textFlags = DTF_ALIGN_TOP|DTF_NO_EFFECTS | ((cfg.msgAlign == 0)? DTF_ALIGN_LEFT : (cfg.msgAlign == 2)? DTF_ALIGN_RIGHT : 0);

    DGL_Enable(DGL_TEXTURE_2D);

    FR_DrawText(str, xOffset, 0, FID(GF_FONTA), textFlags, .5f, 0, cfg.hudColor[CR], cfg.hudColor[CG], cfg.hudColor[CB], textAlpha, 0, 0, false);

    DGL_Disable(DGL_TEXTURE_2D);

    *drawnWidth = FR_TextWidth(chat->buffer.l.l, FID(GF_FONTA)) + FR_CharWidth('_');
    *drawnHeight = MAX_OF(FR_TextHeight(chat->buffer.l.l, FID(GF_FONTA)), FR_CharHeight('_'));
    }
}

/**
 * Sends a string to other player(s) as a chat message.
 */
static void sendMessage(int player, const char* msg)
{
    uiwidget_chat_t* chat = &chatWidgets[player];
    char buff[256];
    int i;

    if(chat->to == 0)
    {   // Send the message to the other players explicitly,
        if(!IS_NETGAME)
        {   // Send it locally.
            for(i = 0; i < MAXPLAYERS; ++i)
            {
                D_NetMessageNoSound(i, msg);
            }
        }
        else
        {
            strcpy(buff, "chat ");
            M_StrCatQuoted(buff, msg, 256);
            DD_Execute(false, buff);
        }
    }
    else
    {   // Send to all of the destination color.
        chat->to -= 1;

        for(i = 0; i < MAXPLAYERS; ++i)
            if(players[i].plr->inGame && cfg.playerColor[i] == chat->to)
            {
                if(!IS_NETGAME)
                {   // Send it locally.
                    D_NetMessageNoSound(i, msg);
                }
                else
                {
                    sprintf(buff, "chatNum %d ", i);
                    M_StrCatQuoted(buff, msg, 256);
                    DD_Execute(false, buff);
                }
            }
    }

#if __JDOOM__
    if(gameModeBits & GM_ANY_DOOM2)
        S_LocalSound(SFX_RADIO, 0);
    else
        S_LocalSound(SFX_TINK, 0);
#elif __JDOOM64__
    S_LocalSound(SFX_RADIO, 0);
#endif
}

boolean Chat_IsActive(int player)
{
    assert(player >= 0 && player < DDMAXPLAYERS);
    {
    uiwidget_chat_t* chat = &chatWidgets[player];
    return chat->active;
    }
}

/**
 * Sets the chat buffer to a chat macro string.
 */
static boolean sendMacro(int player, int num)
{
    if(num >= 0 && num < 9)
    {   // Leave chat mode and notify that it was sent.
        if(chatWidgets[player].active)
            Chat_Open(player, false);

        sendMessage(player, cfg.chatMacros[num]);
        return true;
    }

    return false;
}

/**
 * Handles controls (console commands) for the chat widget.
 */
D_CMD(MsgAction)
{
    int player = CONSOLEPLAYER;
    uiwidget_chat_t* chat = &chatWidgets[player];
    int toPlayer;

    if(G_GetGameAction() == GA_QUIT)
        return false;

    if(chat->active)
    {
        if(!stricmp(argv[0], "chatcomplete"))  // send the message
        {
            Chat_Open(player, false);
            if(chat->buffer.l.len)
            {
                sendMessage(player, chat->buffer.l.l);
            }
        }
        else if(!stricmp(argv[0], "chatcancel"))  // close chat
        {
            Chat_Open(player, false);
        }
        else if(!stricmp(argv[0], "chatdelete"))
        {
            HUlib_delCharFromText(&chat->buffer);
        }
    }

    if(!stricmp(argv[0], "chatsendmacro"))  // send a chat macro
    {
        int macroNum;

        if(argc < 2 || argc > 3)
        {
            Con_Message("Usage: %s (player) (macro number)\n", argv[0]);
            Con_Message("Send a chat macro to other player(s).\n"
                        "If (player) is omitted, the message will be sent to all players.\n");
            return true;
        }

        if(argc == 3)
        {
            toPlayer = atoi(argv[1]);
            if(toPlayer < 0 || toPlayer > 3)
            {
                // Bad destination.
                Con_Message("Invalid player number \"%i\". Should be 0-3\n", toPlayer);
                return false;
            }

            toPlayer = toPlayer + 1;
        }
        else
            toPlayer = 0;

        macroNum = atoi(((argc == 3)? argv[2] : argv[1]));
        if(!sendMacro(player, macroNum))
        {
            Con_Message("Invalid macro number\n");
            return false;
        }
    }
    else if(!stricmp(argv[0], "beginchat")) // begin chat mode
    {
        if(chat->active)
            return false;

        if(argc == 2)
        {
            toPlayer = atoi(argv[1]);
            if(toPlayer < 0 || toPlayer > 3)
            {
                // Bad destination.
                Con_Message("Invalid player number \"%i\". Should be 0-3\n", toPlayer);
                return false;
            }

            toPlayer = toPlayer + 1;
        }
        else
            toPlayer = 0;

        Chat_Open(toPlayer, true);
    }

    return true;
}
