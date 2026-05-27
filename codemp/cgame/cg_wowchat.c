/*
=================================================================================
cg_wowchat.c  —  WoW-style tabbed persistent chat window

Enable with: cg_wowChat 1
Toggle mouse focus: bind a key to "wowchat_toggle"

Tabs:
  [0] General  – all public / team messages
  [1] Console  – server print output
  [2+]         – one tab per DM sender, auto-created

DM detection: messages containing "^7]: ^6" are private (matches existing
server format: ^6^7[SenderName^7]: ^6<text>)
=================================================================================
*/

#include "cg_local.h"
#include "ui/ui_shared.h"

/* ---------- layout (640x480 virtual space) -------------------------------- */
#define WC_TAB_H        16.0f
#define WC_BORDER       1.0f
#define WC_PAD          4.0f
#define WC_FONT_SCALE   0.55f
#define WC_FONT         FONT_SMALL
#define WC_LINE_H       13.0f
#define WC_TAB_W        62.0f
#define WC_ARROW_W      14.0f
#define WC_DRAG_W       18.0f
#define WC_CURSOR_SIZE  16.0f

/* ---------- colours -------------------------------------------------------- */
static vec4_t wc_bg          = { 0.04f, 0.04f, 0.07f, 0.68f };
static vec4_t wc_tabBarBg    = { 0.08f, 0.08f, 0.14f, 0.90f };
static vec4_t wc_tabActive   = { 0.20f, 0.22f, 0.38f, 0.95f };
static vec4_t wc_tabHover    = { 0.14f, 0.14f, 0.24f, 0.90f };
static vec4_t wc_highlight   = { 0.45f, 0.58f, 0.92f, 1.00f };
static vec4_t wc_border      = { 0.28f, 0.28f, 0.48f, 0.78f };
static vec4_t wc_focusBorder = { 0.52f, 0.62f, 0.92f, 0.95f };
static vec4_t wc_dragHandle  = { 0.18f, 0.18f, 0.30f, 0.90f };
static vec4_t wc_text        = { 1.00f, 1.00f, 1.00f, 0.95f };
static vec4_t wc_dimText     = { 0.62f, 0.62f, 0.62f, 0.82f };
static vec4_t wc_unreadBg    = { 0.74f, 0.14f, 0.14f, 1.00f };
static vec4_t wc_scrollText  = { 0.38f, 0.78f, 1.00f, 0.88f };

/* ========================================================================== */
/*  Internal helpers                                                           */
/* ========================================================================== */

static void WC_AddToTab( wowTab_t *tab, const char *text ) {
    Q_strncpyz( tab->lines[tab->head].text, text, sizeof(tab->lines[0].text) );
    tab->head = (tab->head + 1) % WOW_CHAT_HISTORY;
    if ( tab->count < WOW_CHAT_HISTORY )
        tab->count++;
    /* keep the current scroll position stable when new messages arrive */
    if ( tab->scroll > 0 )
        tab->scroll++;
}

static const char *WC_GetLine( const wowTab_t *tab, int fromBottom ) {
    int idx;
    if ( fromBottom < 0 || fromBottom >= tab->count ) return NULL;
    idx = ( (tab->head - 1 - fromBottom) % WOW_CHAT_HISTORY + WOW_CHAT_HISTORY ) % WOW_CHAT_HISTORY;
    return tab->lines[idx].text;
}

/* Extract the clean (no color codes) sender name from a DM string like:
   "^6^7[PlayerName^7]: ^6some message" */
static void WC_ExtractDMSender( const char *msg, char *out, int outSize ) {
    const char *bracket = strstr( msg, "[" );
    const char *end;
    int         len;
    char        raw[MAX_NETNAME];

    if ( !bracket ) { Q_strncpyz( out, "PM", outSize ); return; }
    bracket++; /* skip '[' */

    end = strstr( bracket, "^7]:" );
    if ( !end ) end = strstr( bracket, "]:" );
    if ( !end ) { Q_strncpyz( out, "PM", outSize ); return; }

    len = (int)(end - bracket);
    if ( len <= 0 || len >= MAX_NETNAME ) { Q_strncpyz( out, "PM", outSize ); return; }

    Q_strncpyz( raw, bracket, len + 1 );
    Q_CleanString( raw );
    Q_strncpyz( out, raw, outSize );
}

static int WC_FindOrCreateDMTab( const char *name ) {
    wowChatWindow_t *w = &cg.wowChat;
    int i;

    for ( i = WOW_FIRST_DM_TAB; i < WOW_FIRST_DM_TAB + w->dmCount; i++ ) {
        if ( Q_stricmp( w->tabs[i].name, name ) == 0 )
            return i;
    }
    if ( w->dmCount >= WOW_CHAT_MAX_DM )
        return WOW_TAB_GENERAL; /* fallback: overflow goes to General */

    {
        int idx = WOW_FIRST_DM_TAB + w->dmCount++;
        w->tabs[idx].inUse = qtrue;
        Q_strncpyz( w->tabs[idx].name, name, sizeof(w->tabs[0].name) );
        return idx;
    }
}

/* ========================================================================== */
/*  Public API                                                                 */
/* ========================================================================== */

void CG_WowChat_Init( void ) {
    wowChatWindow_t *w = &cg.wowChat;
    /* cg struct is already zeroed by CG_Init_CG – just set non-zero defaults */
    w->x = 10.0f;
    w->y = 295.0f;
    w->w = 355.0f;
    w->h = 155.0f;

    w->tabs[WOW_TAB_GENERAL].inUse = qtrue;
    Q_strncpyz( w->tabs[WOW_TAB_GENERAL].name, "General", sizeof(w->tabs[0].name) );

    w->tabs[WOW_TAB_CONSOLE].inUse = qtrue;
    Q_strncpyz( w->tabs[WOW_TAB_CONSOLE].name, "Console", sizeof(w->tabs[0].name) );

    w->activeTab = WOW_TAB_GENERAL;
}

void CG_WowChat_AddChat( const char *text ) {
    wowChatWindow_t *w = &cg.wowChat;
    qboolean isDM = (qboolean)( strstr( text, "^7]: ^6" ) != NULL );

    /* every message also goes to General */
    WC_AddToTab( &w->tabs[WOW_TAB_GENERAL], text );
    if ( w->activeTab != WOW_TAB_GENERAL )
        w->tabs[WOW_TAB_GENERAL].unread++;

    if ( isDM ) {
        char sender[MAX_NETNAME];
        int  tabIdx;
        WC_ExtractDMSender( text, sender, sizeof(sender) );
        tabIdx = WC_FindOrCreateDMTab( sender );
        WC_AddToTab( &w->tabs[tabIdx], text );
        if ( w->activeTab != tabIdx )
            w->tabs[tabIdx].unread++;
    }
}

void CG_WowChat_AddConsole( const char *text ) {
    wowChatWindow_t *w = &cg.wowChat;
    char clean[MAX_STRING_CHARS];
    int  len;

    Q_strncpyz( clean, text, sizeof(clean) );
    len = strlen( clean );
    while ( len > 0 && (clean[len-1] == '\n' || clean[len-1] == '\r') )
        clean[--len] = '\0';
    if ( len == 0 ) return;

    WC_AddToTab( &w->tabs[WOW_TAB_CONSOLE], clean );
    if ( w->activeTab != WOW_TAB_CONSOLE )
        w->tabs[WOW_TAB_CONSOLE].unread++;
}

/* Bound to "wowchat_toggle" – toggles mouse focus on the chat window */
void CG_WowChat_FocusToggle_f( void ) {
    wowChatWindow_t *w = &cg.wowChat;
    if ( !cg_wowChat.integer ) return;

    w->focused = (qboolean)(!w->focused);
    if ( w->focused ) {
        /* If cursor is outside the window, snap it to the tab bar */
        if ( cgs.cursorX < w->x || cgs.cursorX > w->x + w->w ||
             cgs.cursorY < w->y || cgs.cursorY > w->y + w->h ) {
            cgs.cursorX = w->x + w->w * 0.5f;
            cgs.cursorY = w->y + WC_TAB_H * 0.5f;
        }
        trap->Key_SetCatcher( trap->Key_GetCatcher() | KEYCATCH_CGAME );
    } else {
        w->dragging = qfalse;
        trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_CGAME );
    }
}

/* Called from CG_KeyEvent when wowChat is focused (handles up AND down events) */
void CG_WowChat_KeyEvent( int key, qboolean down ) {
    wowChatWindow_t *w = &cg.wowChat;
    float cx = cgs.cursorX;
    float cy = cgs.cursorY;
    int   totalTabs   = WOW_FIRST_DM_TAB + w->dmCount;
    float tabAreaW    = w->w - WC_ARROW_W * 2.0f - WC_DRAG_W;
    int   maxVisible  = (int)(tabAreaW / WC_TAB_W);
    if ( maxVisible < 1 ) maxVisible = 1;

    if ( key == A_MOUSE1 ) {
        if ( down ) {
            /* Click outside the window → exit focus */
            if ( cx < w->x || cx > w->x + w->w ||
                 cy < w->y || cy > w->y + w->h ) {
                w->focused  = qfalse;
                w->dragging = qfalse;
                trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_CGAME );
                return;
            }

            if ( cy <= w->y + WC_TAB_H ) {
                /* ---- hit-test the tab bar ---- */
                float tx = w->x;
                int   i;

                /* left arrow */
                if ( cx >= tx && cx < tx + WC_ARROW_W ) {
                    if ( w->tabOffset > 0 ) w->tabOffset--;
                    return;
                }
                tx += WC_ARROW_W;

                /* tab buttons */
                for ( i = w->tabOffset; i < totalTabs && i < w->tabOffset + maxVisible; i++ ) {
                    if ( cx >= tx && cx < tx + WC_TAB_W ) {
                        w->activeTab          = i;
                        w->tabs[i].unread     = 0;
                        return;
                    }
                    tx += WC_TAB_W;
                }

                /* right arrow */
                if ( cx >= tx && cx < tx + WC_ARROW_W ) {
                    if ( w->tabOffset + maxVisible < totalTabs ) w->tabOffset++;
                    return;
                }

                /* drag handle (any remaining area in the tab bar) */
                w->dragging  = qtrue;
                w->dragOffX  = cx - w->x;
                w->dragOffY  = cy - w->y;
            }
            /* click in the message body – keep focus, no other action */
        } else {
            /* mouse button released – end drag */
            w->dragging = qfalse;
        }

    } else if ( key == A_MWHEELUP && down ) {
        w->tabs[w->activeTab].scroll++;

    } else if ( key == A_MWHEELDOWN && down ) {
        if ( w->tabs[w->activeTab].scroll > 0 )
            w->tabs[w->activeTab].scroll--;

    } else if ( key == A_ESCAPE && down ) {
        w->focused  = qfalse;
        w->dragging = qfalse;
        trap->Key_SetCatcher( trap->Key_GetCatcher() & ~KEYCATCH_CGAME );
    }
}

/* Called from CG_MouseEvent with the accumulated absolute cursor position */
void CG_WowChat_MouseMove( float cx, float cy ) {
    wowChatWindow_t *w = &cg.wowChat;
    float nx, ny;
    if ( !w->dragging ) return;

    nx = cx - w->dragOffX;
    ny = cy - w->dragOffY;

    /* clamp to screen */
    if ( nx < 0 )             nx = 0;
    if ( ny < 0 )             ny = 0;
    if ( nx + w->w > 640.0f ) nx = 640.0f - w->w;
    if ( ny + w->h > 480.0f ) ny = 480.0f - w->h;

    w->x = nx;
    w->y = ny;
}

/* ========================================================================== */
/*  Draw                                                                       */
/* ========================================================================== */

void CG_WowChat_Draw( void ) {
    wowChatWindow_t *w = &cg.wowChat;
    float    wx, wy, ww, wh;
    int      totalTabs, maxVisible;
    float    tabAreaW, tx;
    float    cx, cy;
    wowTab_t *activeTab;
    float    bodyTop, bodyBottom, bodyH;
    int      visLines, maxScroll, i;
    float    drawY;

    if ( !cg_wowChat.integer ) return;

    wx = w->x;  wy = w->y;  ww = w->w;  wh = w->h;
    totalTabs = WOW_FIRST_DM_TAB + w->dmCount;
    tabAreaW  = ww - WC_ARROW_W * 2.0f - WC_DRAG_W;
    maxVisible = (int)(tabAreaW / WC_TAB_W);
    if ( maxVisible < 1 ) maxVisible = 1;
    cx = cgs.cursorX;
    cy = cgs.cursorY;

    /* ---- backgrounds ---------------------------------------------------- */
    CG_FillRect( wx, wy + WC_TAB_H, ww, wh - WC_TAB_H, wc_bg );
    CG_FillRect( wx, wy, ww, WC_TAB_H, wc_tabBarBg );

    /* ---- left arrow ----------------------------------------------------- */
    tx = wx;
    {
        qboolean hover  = (qboolean)( w->focused && cx >= tx && cx < tx + WC_ARROW_W &&
                                      cy >= wy && cy < wy + WC_TAB_H );
        qboolean active = (qboolean)( w->tabOffset > 0 );
        if ( hover && active ) CG_FillRect( tx, wy, WC_ARROW_W, WC_TAB_H, wc_tabHover );
        CG_Text_Paint( tx + 3.0f, wy + 4.0f, WC_FONT_SCALE,
                       active ? wc_text : wc_dimText, "<", 0, 0, ITEM_TEXTSTYLE_NORMAL, WC_FONT );
    }
    tx += WC_ARROW_W;

    /* ---- tab buttons ---------------------------------------------------- */
    for ( i = w->tabOffset; i < totalTabs && i < w->tabOffset + maxVisible; i++ ) {
        wowTab_t *tab      = &w->tabs[i];
        qboolean  isActive = (qboolean)( w->activeTab == i );
        qboolean  hover    = (qboolean)( w->focused &&
                                         cx >= tx && cx < tx + WC_TAB_W &&
                                         cy >= wy && cy < wy + WC_TAB_H );
        float        labelW, lx;
        const float *bg = isActive ? wc_tabActive : ( hover ? wc_tabHover : wc_tabBarBg );

        CG_FillRect( tx, wy, WC_TAB_W, WC_TAB_H, bg );

        /* active-tab bottom highlight bar */
        if ( isActive )
            CG_FillRect( tx, wy + WC_TAB_H - 2.0f, WC_TAB_W, 2.0f, wc_highlight );

        /* label – centered, truncated to tab width */
        labelW = CG_Text_Width( tab->name, WC_FONT_SCALE, WC_FONT );
        lx     = tx + (WC_TAB_W - labelW) * 0.5f;
        if ( lx < tx + 2.0f ) lx = tx + 2.0f;
        CG_Text_Paint( lx, wy + 4.0f, WC_FONT_SCALE,
                       isActive ? wc_text : wc_dimText,
                       tab->name, 0, 0, ITEM_TEXTSTYLE_NORMAL, WC_FONT );

        /* unread badge */
        if ( !isActive && tab->unread > 0 ) {
            char   badge[8];
            float  bScale = WC_FONT_SCALE * 0.75f;
            float  bw, bx, by;
            Com_sprintf( badge, sizeof(badge), tab->unread > 99 ? "99+" : "%d",
                         tab->unread > 99 ? 99 : tab->unread );
            bw = CG_Text_Width( badge, bScale, WC_FONT ) + 4.0f;
            bx = tx + WC_TAB_W - bw - 2.0f;
            by = wy + 2.0f;
            CG_FillRect( bx, by, bw, WC_TAB_H - 5.0f, wc_unreadBg );
            CG_Text_Paint( bx + 2.0f, by + 3.0f, bScale,
                           wc_text, badge, 0, 0, ITEM_TEXTSTYLE_NORMAL, WC_FONT );
        }

        /* vertical separator */
        CG_FillRect( tx + WC_TAB_W - 1.0f, wy, 1.0f, WC_TAB_H, wc_border );

        tx += WC_TAB_W;
    }

    /* ---- right arrow ---------------------------------------------------- */
    {
        qboolean hasMore = (qboolean)( w->tabOffset + maxVisible < totalTabs );
        qboolean hover   = (qboolean)( w->focused && cx >= tx && cx < tx + WC_ARROW_W &&
                                       cy >= wy && cy < wy + WC_TAB_H );
        if ( hover && hasMore ) CG_FillRect( tx, wy, WC_ARROW_W, WC_TAB_H, wc_tabHover );
        CG_Text_Paint( tx + 3.0f, wy + 4.0f, WC_FONT_SCALE,
                       hasMore ? wc_text : wc_dimText, ">", 0, 0, ITEM_TEXTSTYLE_NORMAL, WC_FONT );
    }

    /* ---- drag handle (three dots) --------------------------------------- */
    {
        float dragX = wx + ww - WC_DRAG_W;
        int   d;
        CG_FillRect( dragX, wy, WC_DRAG_W, WC_TAB_H, wc_dragHandle );
        for ( d = 0; d < 3; d++ )
            CG_FillRect( dragX + 4.0f + (float)d * 4.5f,
                         wy + WC_TAB_H * 0.5f - 1.5f, 3.0f, 3.0f, wc_dimText );
    }

    /* ---- outer border --------------------------------------------------- */
    CG_DrawRect( wx, wy, ww, wh, WC_BORDER,
                 w->focused ? wc_focusBorder : wc_border );

    /* ---- messages ------------------------------------------------------- */
    activeTab   = &w->tabs[w->activeTab];
    bodyTop     = wy + WC_TAB_H + WC_PAD;
    bodyBottom  = wy + wh - WC_PAD;
    bodyH       = bodyBottom - bodyTop;
    visLines    = (int)(bodyH / WC_LINE_H);
    if ( visLines < 1 ) visLines = 1;

    /* clamp scroll to valid range */
    maxScroll = activeTab->count - visLines;
    if ( maxScroll < 0 ) maxScroll = 0;
    if ( activeTab->scroll > maxScroll ) activeTab->scroll = maxScroll;

    /* draw newest-of-visible at the bottom, older lines above */
    drawY = bodyBottom - WC_LINE_H;
    for ( i = activeTab->scroll;
          i < activeTab->count && i < activeTab->scroll + visLines;
          i++ ) {
        const char *line = WC_GetLine( activeTab, i );
        if ( !line || line[0] == '\0' ) break;
        CG_Text_Paint( wx + WC_PAD, drawY, WC_FONT_SCALE,
                       wc_text, line, 0, 0, ITEM_TEXTSTYLE_NORMAL, WC_FONT );
        drawY -= WC_LINE_H;
        if ( drawY < bodyTop ) break;
    }

    /* "newer messages" indicator when scrolled up */
    if ( activeTab->scroll > 0 ) {
        char badge[24];
        float bScale = WC_FONT_SCALE * 0.75f;
        float bw;
        Com_sprintf( badge, sizeof(badge), "[ +%d newer ]", activeTab->scroll );
        bw = CG_Text_Width( badge, bScale, WC_FONT ) + 4.0f;
        CG_FillRect( wx + ww - bw - WC_PAD - 1.0f,
                     bodyBottom - WC_LINE_H + 1.0f,
                     bw + 2.0f, WC_LINE_H - 2.0f, wc_bg );
        CG_Text_Paint( wx + ww - bw - WC_PAD + 1.0f,
                       bodyBottom - 3.0f, bScale,
                       wc_scrollText, badge, 0, 0, ITEM_TEXTSTYLE_NORMAL, WC_FONT );
    }

    /* ---- software cursor (when focused) --------------------------------- */
    if ( w->focused && cgs.media.cursor ) {
        trap->R_SetColor( NULL );
        CG_DrawPic( cx, cy, WC_CURSOR_SIZE, WC_CURSOR_SIZE, cgs.media.cursor );
    }
}
