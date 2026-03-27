/*
===========================================================================
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// cg_cmdmenu.c — searchable in-game command/cvar overlay menu

#include "cg_local.h"
#include "ui/keycodes.h"
#include "ui/menudef.h"

// ──────────────────────────────────────────────────────────────────────────────
// Entry table
// ──────────────────────────────────────────────────────────────────────────────

typedef enum {
	CMDMENU_CMD,   // execute immediately on Enter
	CMDMENU_CVAR,  // show current value; Enter opens value-edit mode
} cmdMenuEntryType_t;

typedef struct {
	const char           *name;
	const char           *description;
	cmdMenuEntryType_t    type;
} cmdMenuEntry_t;

static const cmdMenuEntry_t s_entries[] = {
	// ── cgame commands ────────────────────────────────────────────────────────
	{ "cmdmenu",              "Open/close this search menu",                             CMDMENU_CMD },
	{ "noclip",               "Toggle noclip (fly through walls)",                       CMDMENU_CMD },
	{ "kill",                 "Kill yourself",                                            CMDMENU_CMD },
	{ "bow",                  "Bow emote",                                                CMDMENU_CMD },
	{ "flourish",             "Flourish emote",                                           CMDMENU_CMD },
	{ "taunt",                "Taunt emote",                                              CMDMENU_CMD },
	{ "gloat",                "Gloat emote",                                              CMDMENU_CMD },
	{ "engage_duel",          "Challenge nearest player to duel",                         CMDMENU_CMD },
	{ "engage_fullforceduel", "Challenge nearest player to full-force duel",              CMDMENU_CMD },
	{ "saberAttackCycle",     "Cycle saber attack style",                                 CMDMENU_CMD },
	{ "force_speed",          "Activate Force Speed",                                     CMDMENU_CMD },
	{ "force_heal",           "Activate Force Heal",                                      CMDMENU_CMD },
	{ "force_healother",      "Heal targeted player with Force",                          CMDMENU_CMD },
	{ "force_absorb",         "Activate Force Absorb",                                    CMDMENU_CMD },
	{ "force_protect",        "Activate Force Protection",                                CMDMENU_CMD },
	{ "force_rage",           "Activate Force Rage",                                      CMDMENU_CMD },
	{ "+force_grip",          "Hold to Force Grip",                                       CMDMENU_CMD },
	{ "+force_lightning",     "Hold to use Force Lightning",                              CMDMENU_CMD },
	{ "+force_drain",         "Hold to use Force Drain",                                  CMDMENU_CMD },
	{ "force_throw",          "Force Throw",                                              CMDMENU_CMD },
	{ "force_pull",           "Force Pull",                                               CMDMENU_CMD },
	{ "force_distract",       "Force Distract",                                           CMDMENU_CMD },
	{ "force_seeing",         "Force Seeing",                                             CMDMENU_CMD },
	{ "weapon 1",             "Switch to weapon 1 (melee)",                               CMDMENU_CMD },
	{ "weapon 2",             "Switch to weapon 2 (saber)",                               CMDMENU_CMD },
	{ "weapon 10",            "Switch to weapon 10 (bryar pistol)",                       CMDMENU_CMD },
	{ "weapprev",             "Previous weapon",                                          CMDMENU_CMD },
	{ "weapnext",             "Next weapon",                                              CMDMENU_CMD },
	{ "invprev",              "Previous inventory item",                                  CMDMENU_CMD },
	{ "invnext",              "Next inventory item",                                      CMDMENU_CMD },
	{ "say",                  "Say to all (append message)",                              CMDMENU_CMD },
	{ "say_team",             "Say to team (append message)",                             CMDMENU_CMD },
	{ "messagemode",          "Open chat to all",                                         CMDMENU_CMD },
	{ "messagemode2",         "Open chat to team",                                        CMDMENU_CMD },
	{ "messagemode3",         "Open private chat",                                        CMDMENU_CMD },
	{ "clientlist",           "List connected clients",                                   CMDMENU_CMD },
	{ "clientlistInfo",       "List connected clients with info",                         CMDMENU_CMD },
	{ "modversion",           "Print mod version",                                        CMDMENU_CMD },
	{ "viewpos",              "Print current position and angles",                        CMDMENU_CMD },
	{ "saber",                "Set saber style (append saber name)",                      CMDMENU_CMD },
	{ "saberColor",           "Set saber color",                                          CMDMENU_CMD },
	{ "amColor",              "Set admin saber color",                                    CMDMENU_CMD },
	{ "amrun",                "Toggle AM run",                                            CMDMENU_CMD },
	{ "login",                "Login to server",                                          CMDMENU_CMD },
	{ "follow",               "Spectate a player (append name)",                          CMDMENU_CMD },
	{ "teleMark",             "Set teleport mark at current position",                    CMDMENU_CMD },
	{ "teleSelfToMark",       "Teleport self to mark",                                    CMDMENU_CMD },
	{ "teleTargetToMark",     "Teleport crosshair target to mark",                        CMDMENU_CMD },
	{ "teleCrosshair",        "Teleport to crosshair",                                    CMDMENU_CMD },
	{ "teleport",             "Teleport by offset (append x y z)",                        CMDMENU_CMD },
	{ "teleToCrosshairWithDist", "Teleport to crosshair keeping distance (append dist)", CMDMENU_CMD },
	{ "teleRespawnMark",      "Teleport to respawn mark",                                 CMDMENU_CMD },
	{ "teleRespawnMarkClear", "Clear respawn mark",                                       CMDMENU_CMD },
	{ "get",                  "Teleport crosshair player to you",                         CMDMENU_CMD },
	{ "teleFrag",             "Telefrag a player",                                        CMDMENU_CMD },
	{ "teleFragSelf",         "Telefrag self",                                            CMDMENU_CMD },
	{ "mimic",                "Start/stop mimicking a player",                            CMDMENU_CMD },
	{ "mimicMirror",          "Mirror mimic mode",                                        CMDMENU_CMD },
	{ "flipkick",             "Flip kick",                                                CMDMENU_CMD },
	{ "lowjump",              "Low jump",                                                  CMDMENU_CMD },
	{ "strafeHelper",         "Strafe helper settings",                                   CMDMENU_CMD },
	{ "speedometer",          "Speedometer settings",                                     CMDMENU_CMD },
	{ "cosmetics",            "Cosmetics menu",                                           CMDMENU_CMD },
	{ "chatlog",              "Chat log settings",                                        CMDMENU_CMD },
	{ "plugin",               "Plugin disable settings",                                  CMDMENU_CMD },
	{ "pluginDisable",        "Plugin disable settings",                                  CMDMENU_CMD },
	{ "stylePlayer",          "Style a player",                                           CMDMENU_CMD },
	{ "remapShader",          "Remap a shader (append oldShader newShader)",               CMDMENU_CMD },
	{ "listRemaps",           "List active shader remaps",                                CMDMENU_CMD },
	{ "listEmojis",           "List available emojis",                                    CMDMENU_CMD },
	{ "autoLogin",            "Auto login settings",                                      CMDMENU_CMD },
	{ "serverconfig",         "Show server config",                                       CMDMENU_CMD },
	{ "do",                   "Repeat a command on a timer (append cmd delay_ms)",        CMDMENU_CMD },
	{ "doStop",               "Stop a running do loop",                                   CMDMENU_CMD },
	{ "amTeleOffset",         "Admin teleport offset",                                    CMDMENU_CMD },
	{ "PTelemark",            "Set P-teleport mark",                                      CMDMENU_CMD },
	{ "PTele",                "P-teleport to mark",                                       CMDMENU_CMD },
	{ "Olol_Info",            "Print Olol plugin info",                                   CMDMENU_CMD },
	{ "loadhud",              "Reload HUD",                                               CMDMENU_CMD },
	{ "sizeup",               "Increase view size",                                       CMDMENU_CMD },
	{ "sizedown",             "Decrease view size",                                       CMDMENU_CMD },

	// ── cvars ─────────────────────────────────────────────────────────────────
	{ "cg_thirdperson",       "Toggle third-person view (0/1)",                           CMDMENU_CVAR },
	{ "cg_thirdPersonRange",  "Third-person camera distance",                             CMDMENU_CVAR },
	{ "cg_thirdPersonAngle",  "Third-person horizontal angle",                            CMDMENU_CVAR },
	{ "cg_thirdPersonVertOffset", "Third-person vertical offset",                         CMDMENU_CVAR },
	{ "cg_thirdPersonHorzOffset", "Third-person horizontal offset",                       CMDMENU_CVAR },
	{ "cg_fov",               "Field of view",                                            CMDMENU_CVAR },
	{ "cg_fovViewmodel",      "Weapon field of view",                                     CMDMENU_CVAR },
	{ "cg_drawFPS",           "Show FPS counter (0=off 1=fps 2=fps+ms)",                  CMDMENU_CVAR },
	{ "cg_drawTimer",         "Show match timer (0=off 1=on 2=extended)",                 CMDMENU_CVAR },
	{ "cg_drawScore",         "Show score display",                                       CMDMENU_CVAR },
	{ "cg_lagometer",         "Lagometer display mode",                                   CMDMENU_CVAR },
	{ "cg_speedometer",       "Speedometer display flags",                                CMDMENU_CVAR },
	{ "cg_strafeHelper",      "Strafe helper display flags",                              CMDMENU_CVAR },
	{ "cg_blood",             "Show blood effects (0/1)",                                 CMDMENU_CVAR },
	{ "cg_screenShake",       "Screen shake on damage (0/1)",                             CMDMENU_CVAR },
	{ "cg_autoswitch",        "Auto-switch weapon on pickup (0/1)",                       CMDMENU_CVAR },
	{ "cg_autoAmneo",         "Auto-amneo on damage (0/1)",                               CMDMENU_CVAR },
	{ "cg_chatBeep",          "Chat notification sound (0/1)",                            CMDMENU_CVAR },
	{ "cg_teamChatBeep",      "Team chat notification sound (0/1)",                       CMDMENU_CVAR },
	{ "cg_chatBoxLines",      "Number of chat lines visible",                             CMDMENU_CVAR },
	{ "cg_chatBoxHeight",     "Chat box pixel height",                                    CMDMENU_CVAR },
	{ "cg_chatBoxEmojis",     "Show emojis in chat box (0/1)",                            CMDMENU_CVAR },
	{ "cg_smallScoreboard",   "Use compact scoreboard (0/1)",                             CMDMENU_CVAR },
	{ "cg_drawScoreboardIcons","Show icons on scoreboard (0/1)",                          CMDMENU_CVAR },
	{ "cg_drawRewards",       "Show kill reward announcements (0/1)",                     CMDMENU_CVAR },
	{ "cg_hudFiles",          "HUD file set to use",                                      CMDMENU_CVAR },
	{ "cg_weaponBob",         "Weapon bob while moving (0/1)",                            CMDMENU_CVAR },
	{ "cg_weaponLag",         "Weapon lag factor",                                        CMDMENU_CVAR },
	{ "cg_fallingBob",        "Camera bob when falling (0/1)",                            CMDMENU_CVAR },
	{ "cg_smoothClients",     "Smooth other players movement (0/1)",                      CMDMENU_CVAR },
	{ "cg_smoothCamera",      "Smooth spectator camera (0/1)",                            CMDMENU_CVAR },
	{ "cg_deferPlayers",      "Defer player model loading (0/1)",                         CMDMENU_CVAR },
	{ "cg_snapshotTimeout",   "Snapshot timeout in seconds",                              CMDMENU_CVAR },
	{ "cg_spectatorCameraDamp","Spectator free-roam camera damping",                      CMDMENU_CVAR },
	{ "cg_freeRoamMode",      "Free-roam spectator camera mode (0/1)",                    CMDMENU_CVAR },
	{ "cg_instaKillOnDamage", "Show instakill effect on taking damage (0/1)",             CMDMENU_CVAR },
	{ "cg_talkState",         "Talk state (0=normal 1=always talk)",                      CMDMENU_CVAR },
	{ "cg_leadIndicator",     "Show race lead indicator (0/1)",                           CMDMENU_CVAR },
	{ "cg_scoreboardBots",    "Show bots on scoreboard (0/1)",                            CMDMENU_CVAR },
	{ "cg_drawHolsteredSaber","Show holstered saber on model (0/1)",                      CMDMENU_CVAR },
	{ "cg_allowMemeVGS",      "Allow meme VGS sounds (0/1)",                              CMDMENU_CVAR },
	{ "cg_alwaysShowAbsorb",  "Always show absorb shield (0/1)",                          CMDMENU_CVAR },
	{ "cg_autoHeal",          "Auto-heal when idle (0/1)",                                CMDMENU_CVAR },
	{ "cl_maxcmdrate",        "Max command/packet rate per second (0=unlimited)",          CMDMENU_CVAR },
	{ "cl_maxpackets",        "Max packets sent per second",                              CMDMENU_CVAR },
	{ "com_maxfps",           "Maximum frame rate",                                       CMDMENU_CVAR },
	{ "s_volume",             "Sound effects volume (0.0–1.0)",                           CMDMENU_CVAR },
	{ "s_musicvolume",        "Music volume (0.0–1.0)",                                   CMDMENU_CVAR },
	{ "s_volumeVoice",        "Voice volume (0.0–1.0)",                                   CMDMENU_CVAR },
	{ "name",                 "Player display name",                                      CMDMENU_CVAR },
	{ "model",                "Player model (model/skin)",                                CMDMENU_CVAR },
	{ "color1",               "Primary saber color",                                      CMDMENU_CVAR },
	{ "color2",               "Secondary saber color",                                    CMDMENU_CVAR },
	{ "saber1",               "Primary saber style",                                      CMDMENU_CVAR },
	{ "forcepowers",          "Force powers string",                                       CMDMENU_CVAR },
	{ "restricts",            "Restriction flags",                                        CMDMENU_CVAR },
	{ "sv_maxcmdrate",        "Server-enforced max command rate (0=no override)",          CMDMENU_CVAR },
	{ "sv_maxTeamSize",       "Max team size (0=unlimited)",                              CMDMENU_CVAR },
	{ "cp_pluginDisable",     "Plugin disable bitmask",                                   CMDMENU_CVAR },
	{ "cl_renderer",          "Renderer backend (rd-vulkan / rd-vanilla)",                CMDMENU_CVAR },
	{ "cl_discordRichPresence","Discord rich presence (0/1)",                             CMDMENU_CVAR },
	{ "cl_afkTime",           "AFK timeout in seconds (0=disabled)",                      CMDMENU_CVAR },
};

#define CMDMENU_ENTRY_COUNT ((int)(sizeof(s_entries) / sizeof(s_entries[0])))

// ──────────────────────────────────────────────────────────────────────────────
// Menu state
// ──────────────────────────────────────────────────────────────────────────────

#define CMDMENU_FILTER_MAX   64
#define CMDMENU_VALUE_MAX    128
#define CMDMENU_VISIBLE      14   // rows shown at once

typedef struct {
	qboolean open;

	// filter/search input
	char     filter[CMDMENU_FILTER_MAX];
	int      filterLen;

	// navigation
	int      selected;   // index into filteredIdx[]
	int      scroll;     // first visible index

	// cvar value-edit mode
	qboolean valueMode;
	char     valueStr[CMDMENU_VALUE_MAX];
	int      valueLen;

	// cached filter results
	int      filteredIdx[CMDMENU_ENTRY_COUNT];
	int      filteredCount;
} cmdMenuState_t;

static cmdMenuState_t s_menu;

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

// Case-insensitive substring search (Q_stristr not available in this codebase)
static qboolean CG_CmdMenuContains( const char *haystack, const char *needle ) {
	int nLen, hLen, i, j;
	if ( !needle || !needle[0] ) return qtrue;
	if ( !haystack ) return qfalse;
	nLen = (int)strlen( needle );
	hLen = (int)strlen( haystack );
	for ( i = 0; i <= hLen - nLen; i++ ) {
		for ( j = 0; j < nLen; j++ ) {
			if ( tolower( (unsigned char)haystack[i+j] ) != tolower( (unsigned char)needle[j] ) )
				break;
		}
		if ( j == nLen ) return qtrue;
	}
	return qfalse;
}

static void CG_CmdMenuRebuildFilter( void ) {
	int i;
	s_menu.filteredCount = 0;

	for ( i = 0; i < CMDMENU_ENTRY_COUNT; i++ ) {
		if ( !s_menu.filter[0] ||
		     CG_CmdMenuContains( s_entries[i].name, s_menu.filter ) ||
		     CG_CmdMenuContains( s_entries[i].description, s_menu.filter ) )
		{
			s_menu.filteredIdx[s_menu.filteredCount++] = i;
		}
	}

	// clamp selection
	if ( s_menu.selected >= s_menu.filteredCount ) {
		s_menu.selected = Q_max( 0, s_menu.filteredCount - 1 );
	}
	// clamp scroll
	if ( s_menu.scroll > s_menu.selected ) {
		s_menu.scroll = s_menu.selected;
	}
	if ( s_menu.scroll + CMDMENU_VISIBLE <= s_menu.selected ) {
		s_menu.scroll = s_menu.selected - CMDMENU_VISIBLE + 1;
	}
}

static void CG_CmdMenuOpen( void ) {
	memset( &s_menu, 0, sizeof( s_menu ) );
	s_menu.open = qtrue;
	CG_CmdMenuRebuildFilter();
	trap->Key_SetCatcher( KEYCATCH_CGAME );
}

void CG_CmdMenuClose( void ) {
	s_menu.open = qfalse;
	trap->Key_SetCatcher( 0 );
}

static void CG_CmdMenuExecuteSelected( void ) {
	const cmdMenuEntry_t *e;
	int idx;

	if ( s_menu.filteredCount == 0 ) return;
	idx = s_menu.filteredIdx[s_menu.selected];
	e   = &s_entries[idx];

	if ( e->type == CMDMENU_CMD ) {
		CG_CmdMenuClose();
		trap->SendConsoleCommand( va( "%s\n", e->name ) );
	} else {
		// enter value-edit mode
		s_menu.valueMode = qtrue;
		trap->Cvar_VariableStringBuffer( e->name, s_menu.valueStr, sizeof( s_menu.valueStr ) );
		s_menu.valueLen = (int)strlen( s_menu.valueStr );
	}
}

static void CG_CmdMenuCommitValue( void ) {
	int idx;
	if ( s_menu.filteredCount == 0 ) return;
	idx = s_menu.filteredIdx[s_menu.selected];
	trap->SendConsoleCommand( va( "%s \"%s\"\n", s_entries[idx].name, s_menu.valueStr ) );
	s_menu.valueMode = qfalse;
}

// ──────────────────────────────────────────────────────────────────────────────
// Public: toggle
// ──────────────────────────────────────────────────────────────────────────────

void CG_CmdMenu_f( void ) {
	if ( s_menu.open ) {
		CG_CmdMenuClose();
	} else {
		CG_CmdMenuOpen();
	}
}

qboolean CG_CmdMenuIsOpen( void ) {
	return s_menu.open;
}

// ──────────────────────────────────────────────────────────────────────────────
// Public: key events
// ──────────────────────────────────────────────────────────────────────────────

void CG_CmdMenuKeyEvent( int key ) {
	char ch;

	if ( !s_menu.open ) return;

	// ── character input (K_CHAR_FLAG set by engine) ───────────────────────────
	if ( key & K_CHAR_FLAG ) {
		ch = (char)( key & ~K_CHAR_FLAG );

		if ( s_menu.valueMode ) {
			if ( ch >= 0x20 && s_menu.valueLen < CMDMENU_VALUE_MAX - 1 ) {
				s_menu.valueStr[s_menu.valueLen++] = ch;
				s_menu.valueStr[s_menu.valueLen]   = '\0';
			}
		} else {
			if ( ch >= 0x20 && s_menu.filterLen < CMDMENU_FILTER_MAX - 1 ) {
				s_menu.filter[s_menu.filterLen++] = ch;
				s_menu.filter[s_menu.filterLen]   = '\0';
				s_menu.selected = 0;
				s_menu.scroll   = 0;
				CG_CmdMenuRebuildFilter();
			}
		}
		return;
	}

	// ── navigation / control keys ─────────────────────────────────────────────
	switch ( key ) {
	case A_ESCAPE:
		if ( s_menu.valueMode ) {
			s_menu.valueMode = qfalse;
		} else {
			CG_CmdMenuClose();
		}
		break;

	case A_ENTER:
	case A_KP_ENTER:
		if ( s_menu.valueMode ) {
			CG_CmdMenuCommitValue();
		} else {
			CG_CmdMenuExecuteSelected();
		}
		break;

	case A_BACKSPACE:
		if ( s_menu.valueMode ) {
			if ( s_menu.valueLen > 0 ) {
				s_menu.valueStr[--s_menu.valueLen] = '\0';
			}
		} else {
			if ( s_menu.filterLen > 0 ) {
				s_menu.filter[--s_menu.filterLen] = '\0';
				s_menu.selected = 0;
				s_menu.scroll   = 0;
				CG_CmdMenuRebuildFilter();
			}
		}
		break;

	case A_CURSOR_UP:
		if ( !s_menu.valueMode && s_menu.selected > 0 ) {
			s_menu.selected--;
			if ( s_menu.selected < s_menu.scroll ) {
				s_menu.scroll = s_menu.selected;
			}
		}
		break;

	case A_CURSOR_DOWN:
		if ( !s_menu.valueMode && s_menu.selected < s_menu.filteredCount - 1 ) {
			s_menu.selected++;
			if ( s_menu.selected >= s_menu.scroll + CMDMENU_VISIBLE ) {
				s_menu.scroll = s_menu.selected - CMDMENU_VISIBLE + 1;
			}
		}
		break;

	case A_PAGE_UP:
		if ( !s_menu.valueMode ) {
			s_menu.selected = Q_max( 0, s_menu.selected - CMDMENU_VISIBLE );
			s_menu.scroll   = Q_max( 0, s_menu.scroll   - CMDMENU_VISIBLE );
		}
		break;

	case A_PAGE_DOWN:
		if ( !s_menu.valueMode ) {
			s_menu.selected = Q_min( s_menu.filteredCount - 1, s_menu.selected + CMDMENU_VISIBLE );
			if ( s_menu.selected >= s_menu.scroll + CMDMENU_VISIBLE ) {
				s_menu.scroll = s_menu.selected - CMDMENU_VISIBLE + 1;
			}
		}
		break;

	default:
		break;
	}
}

// ──────────────────────────────────────────────────────────────────────────────
// Public: draw
// ──────────────────────────────────────────────────────────────────────────────

// Match the chat font: FONT_SMALL at 0.65 scale.
// CHATBOX_FONT_HEIGHT (20) * 0.65 = 13px per line at this scale.
#define MENU_X        80.0f
#define MENU_Y        40.0f
#define MENU_W        480.0f
#define TEXT_SCALE    0.65f   // chat font scale
#define HINT_SCALE    0.50f   // smaller for hints
#define ROW_H         16.0f   // row height: text (~13px) + 3px padding
#define HEADER_H      38.0f   // title line + filter line
#define FOOTER_H      30.0f   // description + hint line

void CG_CmdMenuDraw( void ) {
	int   i, visEnd;
	float y, textH, textPad;
	vec4_t bg           = { 0.05f, 0.05f, 0.10f, 0.88f };
	vec4_t headerBg     = { 0.10f, 0.10f, 0.22f, 0.95f };
	vec4_t selBg        = { 0.20f, 0.35f, 0.60f, 0.80f };
	vec4_t cmdColor     = { 1.00f, 1.00f, 1.00f, 1.00f };
	vec4_t cvarColor    = { 0.80f, 0.90f, 1.00f, 1.00f };
	vec4_t descColor    = { 0.65f, 0.65f, 0.65f, 1.00f };
	vec4_t filterColor  = { 1.00f, 1.00f, 0.60f, 1.00f };
	vec4_t valueColor   = { 0.60f, 1.00f, 0.60f, 1.00f };
	vec4_t hintColor    = { 0.50f, 0.50f, 0.50f, 1.00f };
	vec4_t borderColor  = { 0.30f, 0.50f, 0.80f, 0.70f };
	float  menuH;
	char   valueBuf[CMDMENU_VALUE_MAX];
	const cmdMenuEntry_t *selEntry = NULL;

	if ( !s_menu.open ) return;

	// Measure actual text height for this font/scale to center in rows
	textH   = (float)CG_Text_Height( "A", TEXT_SCALE, FONT_SMALL );
	textPad = (ROW_H - textH) * 0.5f;

	menuH = HEADER_H + (CMDMENU_VISIBLE * ROW_H) + FOOTER_H;

	// ── background ────────────────────────────────────────────────────────────
	CG_FillRect( MENU_X - 1, MENU_Y - 1, MENU_W + 2, menuH + 2, borderColor );
	CG_FillRect( MENU_X, MENU_Y, MENU_W, menuH, bg );

	// ── header: title + filter input ──────────────────────────────────────────
	CG_FillRect( MENU_X, MENU_Y, MENU_W, HEADER_H, headerBg );

	// Title at top of header, slightly smaller than chat scale
	CG_Text_Paint( MENU_X + 6, MENU_Y + 2, HINT_SCALE, cmdColor,
	               "Command Search  (Esc to close)", 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );

	// Filter input line using full chat scale
	{
		char filterDisplay[CMDMENU_FILTER_MAX + 4];
		if ( s_menu.filter[0] ) {
			Com_sprintf( filterDisplay, sizeof( filterDisplay ), "> %s_", s_menu.filter );
		} else {
			Q_strncpyz( filterDisplay, "> _", sizeof( filterDisplay ) );
		}
		CG_Text_Paint( MENU_X + 6, MENU_Y + HEADER_H - ROW_H + textPad, TEXT_SCALE, filterColor,
		               filterDisplay, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}

	// Result count (right-aligned, hint scale)
	{
		char countStr[32];
		Com_sprintf( countStr, sizeof( countStr ), "%d / %d", s_menu.filteredCount, CMDMENU_ENTRY_COUNT );
		CG_Text_Paint( MENU_X + MENU_W - CG_Text_Width( countStr, HINT_SCALE, FONT_SMALL ) - 6,
		               MENU_Y + HEADER_H - ROW_H + textPad + (textH - (float)CG_Text_Height( countStr, HINT_SCALE, FONT_SMALL )) * 0.5f,
		               HINT_SCALE, hintColor, countStr, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}

	// ── list ──────────────────────────────────────────────────────────────────
	y = MENU_Y + HEADER_H;
	visEnd = Q_min( s_menu.scroll + CMDMENU_VISIBLE, s_menu.filteredCount );

	for ( i = s_menu.scroll; i < visEnd; i++ ) {
		int entIdx = s_menu.filteredIdx[i];
		const cmdMenuEntry_t *e = &s_entries[entIdx];
		qboolean isSelected = ( i == s_menu.selected );

		if ( isSelected ) {
			CG_FillRect( MENU_X, y, MENU_W, ROW_H, selBg );
			selEntry = e;
		}

		// Entry name: vertically centered in the row
		CG_Text_Paint( MENU_X + 8, y + textPad,
		               TEXT_SCALE, (e->type == CMDMENU_CVAR) ? cvarColor : cmdColor,
		               e->name, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );

		// Cvar: show current value right after the name
		if ( e->type == CMDMENU_CVAR ) {
			trap->Cvar_VariableStringBuffer( e->name, valueBuf, sizeof( valueBuf ) );
			if ( valueBuf[0] ) {
				float nameW = CG_Text_Width( e->name, TEXT_SCALE, FONT_SMALL );
				CG_Text_Paint( MENU_X + 8 + nameW + 6, y + textPad,
				               TEXT_SCALE, valueColor, va( "= %s", valueBuf ),
				               0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			}
		}

		y += ROW_H;
	}

	// Empty list notice
	if ( s_menu.filteredCount == 0 ) {
		CG_Text_Paint( MENU_X + 8, MENU_Y + HEADER_H + textPad,
		               TEXT_SCALE, descColor, "No matches",
		               0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
	}

	// ── footer ────────────────────────────────────────────────────────────────
	{
		float footerY  = MENU_Y + HEADER_H + CMDMENU_VISIBLE * ROW_H;
		float hintH    = (float)CG_Text_Height( "A", HINT_SCALE, FONT_SMALL );
		float line1Y   = footerY + (FOOTER_H * 0.5f - hintH) * 0.5f;
		float line2Y   = footerY + FOOTER_H * 0.5f + (FOOTER_H * 0.5f - hintH) * 0.5f;

		CG_FillRect( MENU_X, footerY, MENU_W, FOOTER_H, headerBg );

		if ( s_menu.valueMode && selEntry ) {
			// value-edit mode: takes the full footer
			char valueLine[CMDMENU_VALUE_MAX + 32];
			float vH = (float)CG_Text_Height( "A", TEXT_SCALE, FONT_SMALL );
			Com_sprintf( valueLine, sizeof( valueLine ), "Set %s = %s_", selEntry->name, s_menu.valueStr );
			CG_Text_Paint( MENU_X + 6, footerY + (FOOTER_H - vH) * 0.5f,
			               TEXT_SCALE, valueColor, valueLine,
			               0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
		} else {
			// Line 1: description of selected entry (or a generic hint)
			if ( selEntry && selEntry->description ) {
				CG_Text_Paint( MENU_X + 6, line1Y, HINT_SCALE, descColor,
				               selEntry->description, 0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			}

			// Line 2: key hints
			if ( selEntry ) {
				CG_Text_Paint( MENU_X + 6, line2Y, HINT_SCALE, hintColor,
				               (selEntry->type == CMDMENU_CVAR)
				                   ? "Enter=edit value  Up/Down=navigate  PgUp/PgDn=scroll"
				                   : "Enter=execute  Up/Down=navigate  PgUp/PgDn=scroll",
				               0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			}

			// Scroll indicator (right side, line 2)
			if ( s_menu.filteredCount > CMDMENU_VISIBLE ) {
				char scrollStr[16];
				Com_sprintf( scrollStr, sizeof( scrollStr ), "[%d / %d]",
				             s_menu.selected + 1, s_menu.filteredCount );
				CG_Text_Paint( MENU_X + MENU_W - CG_Text_Width( scrollStr, HINT_SCALE, FONT_SMALL ) - 6,
				               line2Y, HINT_SCALE, hintColor, scrollStr,
				               0.0f, 0, ITEM_TEXTSTYLE_OUTLINED, FONT_SMALL );
			}
		}
	}
}
