/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

#include "../../codemp/client/cl_addonapi.h"

// Import structure
static addonimport_t *ai = NULL;

// Test command function
static void Cmd_TestOlol_f( void ) {
	ai->Printf( "test success\n" );
}

// Info command function
static void Cmd_Olol_Info_f( void ) {
	ai->Printf("^3=== Olol Commands Info ===\n");
	ai->Printf("^5To unlock locked commands, use: ^2helpUsOlol <value>\n\n");

	ai->Printf("^6--- Locked Commands (Password Required) ---\n");
	ai->Printf("^1teleFrag^7 - Teleport to a target player's predicted position (accounts for movement)\n");
	ai->Printf("^1teleFragSelf^7 - Teleport target player to your position\n");
	ai->Printf("^1teleCrosshair^7 - Teleport to where your crosshair is pointing\n");
	ai->Printf("^1teleToCrosshairWithDist^7 - Teleport to crosshair with optional distance offset\n");
	ai->Printf("^1get^7 - Teleport a target player in front of you (with optional offset)\n");
	ai->Printf("^1bring^7 - Teleport the player under your crosshair to your position\n");
	ai->Printf("^1teleport^7 - Teleport with X/Y/Z offset from current or target position\n");
	ai->Printf("^1teleMark^7 - Set a telemark at current position or specified coordinates\n");
	ai->Printf("^1teleTargetToMark^7 - Teleport the target under crosshair to the telemark\n");
	ai->Printf("^1teleSelfToMark^7 - Teleport yourself to the telemark\n");
	ai->Printf("^1teleToMark^7 - Teleport to a player's position (by name or number)\n");
	ai->Printf("^1teleRespawnMark^7 - Set a respawn telemark at current position\n");
	ai->Printf("^1teleRespawnMarkClear^7 - Clear the respawn telemark\n");
	ai->Printf("^1mimic^7 - ^3[For fun - Not finished yet]^7 Mimic another player's movements\n");
	ai->Printf("^1mimicMirror^7 - ^3[For fun - Not finished yet]^7 Mirror mimic another player's movements\n\n");

	ai->Printf("^2--- Unlocked Commands (Always Available) ---\n");
	ai->Printf("^2helpUsOlol^7 - Set password to unlock spicy commands (usage: helpUsOlol <value>)\n");
	ai->Printf("^2Olol_Info^7 - Display this help message\n");
	ai->Printf("^2teleSafeCrosshair^7 - Teleport to crosshair position, avoiding players (finds nearest free spot)\n\n");

	ai->Printf("^5--- Client Variables ---\n");
	ai->Printf("^5cg_autoHeal^7 - Automatically heal when health drops below a threshold\n");
	ai->Printf("^5cg_instaKillOnDamage^7 - Instantly kill when taking any damage\n\n");

	ai->Printf("^4Warning:^7 Abuse of these commands will be monitored and heavily punished.\n");
	ai->Printf("^4Note:^7 Your client activity is trackable and all logs are visible to server admins.\n");
	ai->Printf("^4      ^7Some commands require a password to prevent abuse.\n\n");

	ai->Printf("^6--- Credits ---\n");
	ai->Printf("^3Toxiee and Akr (partial credits): ^3teleFrag, get, bring, teleport\n");
	ai->Printf("^3Olol (full credits): teleFragSelf, teleCrosshair, teleSafeCrosshair, teleToCrosshairWithDist, teleMark,\n");
	ai->Printf("^3                teleTargetToMark, teleSelfToMark, teleToMark, teleRespawnMark, teleRespawnMarkClear,\n");
	ai->Printf("^3                mimic, mimicMirror, helpUsOlol, Olol_Info, cg_autoHeal, cg_instaKillOnDamage\n");
}

/*
============
Addon_Init
============
*/
static qboolean Addon_Init( void ) {
	// Add our test command
	ai->Cmd_AddCommand( "test_olol", Cmd_TestOlol_f, "Test command for Olol addon" );

	// Add the info command
	ai->Cmd_AddCommand( "Olol_Info", Cmd_Olol_Info_f, "Display information about Olol addon commands" );

	ai->Printf( "Olol addon initialized successfully\n" );

	return qtrue;
}

/*
============
Addon_Shutdown
============
*/
static void Addon_Shutdown( qboolean restarting ) {
	// Remove our commands
	ai->Cmd_RemoveCommand( "test_olol" );
	ai->Cmd_RemoveCommand( "Olol_Info" );

	ai->Printf( "Olol addon shut down\n" );
}

// Export structure
static addonexport_t ae = {
	Addon_Shutdown,
	Addon_Init
};

/*
============
GetAddonAPI
============
*/
#ifdef __cplusplus
extern "C" {
#endif

Q_EXPORT addonexport_t* QDECL GetAddonAPI( int apiVersion, addonimport_t *import ) {
	if ( apiVersion != ADDON_API_VERSION ) {
		import->Printf( "Olol addon: API version mismatch (%d != %d)\n", apiVersion, ADDON_API_VERSION );
		return NULL;
	}

	ai = import;

	return &ae;
}

#ifdef __cplusplus
}
#endif