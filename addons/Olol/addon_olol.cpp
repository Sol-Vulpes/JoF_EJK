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
#include "../../codemp/qcommon/q_shared.h"
#include "../../codemp/game/bg_public.h" // for playerState_t, entityState_t
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_STRING_CHARS 1024

// Import structure
static addonimport_t *ai = NULL;

// Minimal implementations of required functions
int QDECL Com_sprintf(char *dest, int size, const char *fmt, ...) {
	va_list argptr;
	va_start(argptr, fmt);
	int len = vsnprintf(dest, size, fmt, argptr);
	va_end(argptr);
	return len;
}

int Q_stricmp(const char *s1, const char *s2) {
	return Q_stricmpn(s1, s2, 99999);
}

int Q_stricmpn(const char *s1, const char *s2, int n) {
	int c1, c2;

	if (s1 == NULL) {
		if (s2 == NULL)
			return 0;
		else
			return -1;
	} else if (s2 == NULL)
		return 1;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point

		if (c1 != c2) {
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

void Q_strncpyz(char *dest, const char *src, int destsize) {
	if (!dest) {
		//Com_Error(ERR_FATAL, "Q_strncpyz: NULL dest");
		return;
	}
	if (!src) {
		//Com_Error(ERR_FATAL, "Q_strncpyz: NULL src");
		return;
	}
	if (destsize < 1) {
		//Com_Error(ERR_FATAL, "Q_strncpyz: destsize < 1");
		return;
	}

	strncpy(dest, src, destsize - 1);
	dest[destsize - 1] = 0;
}

// Helper function stubs (would need real implementations in main cgame)
static qboolean IsPositionFree(vec3_t position) {
	// Stub implementation
	return qtrue;
}

static qboolean FindNearestFreeSpot(vec3_t start, vec3_t result) {
	// Stub implementation - just copy start to result
	VectorCopy(start, result);
	return qtrue;
}

// Global variables for addon state
static qboolean helpUsOlolUnlocked = qfalse;
static float customTelemarkX = 0.0f, customTelemarkY = 0.0f, customTelemarkZ = 0.0f, customTelemarkYaw = 0.0f;
static float respawnTelemarkX = 0.0f, respawnTelemarkY = 0.0f, respawnTelemarkZ = 0.0f, respawnTelemarkYaw = 0.0f;
static qboolean useRespawnTelemark = qfalse;

// Test command function
static void Cmd_TestOlol_f( void ) {
	ai->Printf( "test success\n" );
}

// CG_TeleFrag_f - Teleport to a target player's predicted position (accounts for movement)
static void CG_TeleFrag_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	char argv1[MAX_STRING_CHARS];
	int targetNum = -1;
	const entityState_t* cent;
	vec3_t targetOrigin;
	float pingSec, speed;

	// Get first argument
	if (ai->Cmd_Argc() > 1) {
		Q_strncpyz(argv1, ai->Cmd_Argv(1), sizeof(argv1));
	} else {
		return; // no argument given
	}

	if (!argv1[0]) {
		return; // no argument given
	}

	// Case 1: crosshair
	if (Q_stricmp(argv1, "gun") == 0) {
		targetNum = ai->CrosshairPlayer();
	}
	// Case 2: numeric clientNum
	else if (argv1[0] >= '0' && argv1[0] <= '9') {
		targetNum = atoi(argv1);
	}
	// Case 3: treat as player name
	else {
		targetNum = ai->ClientNumberFromString(argv1);
	}

	if (targetNum < 0) {
		return;
	}

	cent = (const entityState_t*)ai->GetEntityState(targetNum);
	if (!cent) {
		return;
	}

	// Base position - using pos.trBase since we don't have lerpOrigin in addon
	VectorCopy(cent->pos.trBase, targetOrigin);

	// Calculate speed
	speed = VectorLength(cent->pos.trDelta);

	// Get player state for ping and yaw
	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	// Convert ping to seconds, clamp
	pingSec = ps->ping / 1000.0f;
	if (pingSec > 0.25f) pingSec = 0.25f;

	// Apply prediction if moving
	if (speed > 1.0f) {
		targetOrigin[0] += cent->pos.trDelta[0] * pingSec;
		targetOrigin[1] += cent->pos.trDelta[1] * pingSec;
		targetOrigin[2] += cent->pos.trDelta[2] * pingSec;
	}

	// Teleport
	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %f %f %f %f\n",
		targetOrigin[0], targetOrigin[1], targetOrigin[2], ps->viewangles[YAW]);
	ai->SendClientCommand(cmd);
}

// CG_TeleFragSelf_f - Teleport target player to your position
static void CG_TeleFragSelf_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	char argv1[MAX_STRING_CHARS];
	int targetNum = -1;

	// Get first argument
	if (ai->Cmd_Argc() > 1) {
		Q_strncpyz(argv1, ai->Cmd_Argv(1), sizeof(argv1));
	} else {
		return; // no argument given
	}

	if (!argv1[0]) {
		return; // no argument given
	}

	// Case 1: crosshair
	if (Q_stricmp(argv1, "gun") == 0) {
		targetNum = ai->CrosshairPlayer();
	}
	// Case 2: numeric clientNum
	else if (argv1[0] >= '0' && argv1[0] <= '9') {
		targetNum = atoi(argv1);
	}
	// Case 3: treat as player name
	else {
		targetNum = ai->ClientNumberFromString(argv1);
	}

	if (targetNum < 0) {
		return;
	}

	// Get player state
	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	// Teleport the target to your position with your yaw
	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %i %f %f %f %f\n",
		targetNum,
		ps->origin[0], ps->origin[1], ps->origin[2],
		ps->viewangles[YAW]);
	ai->SendClientCommand(cmd);
}

// CG_TeleCrosshair_f - Teleport to where your crosshair is pointing
static void CG_TeleCrosshair_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	// Get player state
	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	// Calculate crosshair position using trace
	vec3_t start, end, forward;
	AngleVectors(ps->viewangles, forward, NULL, NULL);

	VectorCopy(ps->origin, start);
	start[2] += ps->viewheight;

	VectorMA(start, 8192.0f, forward, end);

	trace_t trace;
	ai->Trace(&trace, start, NULL, NULL, end, ps->clientNum, CONTENTS_SOLID);

	vec3_t crosshairPos;
	VectorCopy(trace.endpos, crosshairPos);

	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %f %f %f %f\n",
		crosshairPos[0], crosshairPos[1], crosshairPos[2] + 24, ps->viewangles[YAW]);
	ai->SendClientCommand(cmd);
}

// CG_TeleSafeCrosshair_f - Teleport to crosshair position, avoiding players (finds nearest free spot)
static void CG_TeleSafeCrosshair_f(void) {
	// Get player state
	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	// Calculate crosshair position
	vec3_t start, end, forward;
	AngleVectors(ps->viewangles, forward, NULL, NULL);

	VectorCopy(ps->origin, start);
	start[2] += ps->viewheight;

	VectorMA(start, 8192.0f, forward, end);

	trace_t trace;
	ai->Trace(&trace, start, NULL, NULL, end, ps->clientNum, CONTENTS_SOLID);

	vec3_t newPos;
	VectorCopy(trace.endpos, newPos);
	newPos[2] += 24; // Adjust Z position

	// Check if position is free (simplified check)
	vec3_t mins = {-15, -15, -24};
	vec3_t maxs = {15, 15, 32};
	ai->Trace(&trace, newPos, mins, maxs, newPos, ps->clientNum, CONTENTS_SOLID|CONTENTS_BODY);

	if (trace.startsolid || trace.allsolid) {
		// Position is occupied, find a nearby free spot
		// Simple implementation: try a few offset positions
		vec3_t testPos;
		qboolean found = qfalse;

		for (int dx = -50; dx <= 50 && !found; dx += 25) {
			for (int dy = -50; dy <= 50 && !found; dy += 25) {
				VectorCopy(newPos, testPos);
				testPos[0] += dx;
				testPos[1] += dy;

				ai->Trace(&trace, testPos, mins, maxs, testPos, ps->clientNum, CONTENTS_SOLID|CONTENTS_BODY);
				if (!trace.startsolid && !trace.allsolid) {
					VectorCopy(testPos, newPos);
					found = qtrue;
				}
			}
		}

		if (!found) {
			ai->Printf("No free spot found near crosshair position\n");
			return;
		}
	}

	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %f %f %f %f\n",
		newPos[0], newPos[1], newPos[2], ps->viewangles[YAW]);
	ai->SendClientCommand(cmd);
}

// CG_TeleTargetPlayer_f - Teleport a target player in front of you (with optional offset)
static void CG_TeleTargetPlayer_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	vec3_t viewAngles, forward, newPos;
	int targetNum;
	float offset = 100.0f;

	// Get player state
	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	char arg1[MAX_STRING_CHARS];
	if (ai->Cmd_Argc() > 1) {
		Q_strncpyz(arg1, ai->Cmd_Argv(1), sizeof(arg1));

		if (Q_stricmp(arg1, "gun") == 0) {
			targetNum = ai->CrosshairPlayer();
		} else {
			targetNum = ai->ClientNumberFromString(arg1);
		}

		if (ai->Cmd_Argc() > 2) {
			offset = atof(ai->Cmd_Argv(2));
		}
	} else {
		targetNum = ai->CrosshairPlayer();
	}

	if (targetNum == -1) {
		return;
	}

	// Get view angles and calculate forward direction
	VectorCopy(ps->viewangles, viewAngles);
	AngleVectors(viewAngles, forward, NULL, NULL);

	// Calculate new position in front of the player
	VectorMA(ps->origin, offset, forward, newPos);

	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %d %f %f %f %f\n", targetNum, newPos[0], newPos[1], newPos[2], ps->viewangles[YAW]);
	ai->SendClientCommand(cmd);
}

// CG_TeleCrosshairToMe_f - Teleport the player under your crosshair to your position
static void CG_TeleCrosshairToMe_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	// Get player state
	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	int clientNum = ai->CrosshairPlayer();
	if (clientNum == -1) {
		return;
	}

	// Teleport the player under crosshair to our position
	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %d %f %f %f %f\n", clientNum, ps->origin[0], ps->origin[1], ps->origin[2], ps->viewangles[YAW]);
	ai->SendClientCommand(cmd);
}

// CG_TeleportGun_f - Teleport the player under crosshair with X/Y/Z offset
static void CG_TeleportGun_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	// Get player state
	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	if (ai->Cmd_Argc() != 4) {
		ai->Printf("Usage: teleportGun <offsetX> <offsetY> <offsetZ>\n");
		return;
	}

	int offsetX = atoi(ai->Cmd_Argv(1));
	int offsetY = atoi(ai->Cmd_Argv(2));
	int offsetZ = atoi(ai->Cmd_Argv(3));

	int targetNum = ai->CrosshairPlayer();
	if (targetNum == -1) {
		ai->Printf("No player under crosshair!\n");
		return;
	}

	const entityState_t* cent = (const entityState_t*)ai->GetEntityState(targetNum);
	if (!cent) {
		ai->Printf("Could not get target player state!\n");
		return;
	}

	vec3_t targetOrigin;
	VectorCopy(cent->pos.trBase, targetOrigin);

	int teleOffsetX = targetOrigin[0] + offsetX;
	int teleOffsetY = targetOrigin[1] + offsetY;
	int teleOffsetZ = targetOrigin[2] + offsetZ;
	int yaw = ps->viewangles[YAW];

	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %i %i %i %i %i\n", targetNum, teleOffsetX, teleOffsetY, teleOffsetZ, yaw);
	ai->SendClientCommand(cmd);
}

// CG_PTele_Offset_f - Teleport with X/Y/Z offset from current position or target player
static void CG_PTele_Offset_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	// Get player state
	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	int x, y, z, yaw, offsetX, offsetY, offsetZ, teleOffsetX, teleOffsetY, teleOffsetZ, targetNum;
	float barrier = 50; // Prevent telecrush - barrier

	offsetX = 0;
	offsetY = 0;
	offsetZ = 0;

	if (ai->Cmd_Argc() == 4) {
		offsetX = atoi(ai->Cmd_Argv(1));
		offsetY = atoi(ai->Cmd_Argv(2));
		offsetZ = atoi(ai->Cmd_Argv(3));

		x = ps->origin[0];
		y = ps->origin[1];
		z = ps->origin[2];
		yaw = ps->viewangles[YAW];

		teleOffsetX = x + offsetX;
		teleOffsetY = y + offsetY;
		teleOffsetZ = z + offsetZ;

		char cmd[256];
		Com_sprintf(cmd, sizeof(cmd), "amtele %i %i %i %i\n", teleOffsetX, teleOffsetY, teleOffsetZ, yaw);
		ai->SendClientCommand(cmd);
	}

	// If there's a 5th argument, interpret it as target player name/number
	if (ai->Cmd_Argc() == 5) {
		offsetX = atoi(ai->Cmd_Argv(1));
		offsetY = atoi(ai->Cmd_Argv(2));
		offsetZ = atoi(ai->Cmd_Argv(3));

		char argv5[MAX_STRING_CHARS];
		Q_strncpyz(argv5, ai->Cmd_Argv(4), sizeof(argv5));

		targetNum = ai->ClientNumberFromString(argv5);
		if (targetNum != -1) {
			const entityState_t* cent = (const entityState_t*)ai->GetEntityState(targetNum);
			if (cent) {
				vec3_t targetOrigin;
				VectorCopy(cent->pos.trBase, targetOrigin);
				yaw = ps->viewangles[YAW];

				teleOffsetX = targetOrigin[0] + offsetX;
				teleOffsetY = targetOrigin[1] + offsetY;
				teleOffsetZ = targetOrigin[2] + offsetZ;

				char cmd[256];
				Com_sprintf(cmd, sizeof(cmd), "amtele %i %i %i %i %i\n", targetNum, teleOffsetX, teleOffsetY, teleOffsetZ, yaw);
				ai->SendClientCommand(cmd);
			}
		}
	}
}

// CG_TeleportToCrosshairWithDistance_f - Teleport to crosshair with optional distance offset
static void CG_TeleportToCrosshairWithDistance_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	// Get player state
	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	vec3_t current_pos, forward, crosshair_pos, new_pos;
	float added_distance = 0.0f;

	if (ai->Cmd_Argc() == 2) {
		added_distance = atof(ai->Cmd_Argv(1));
	}

	VectorCopy(ps->origin, current_pos);
	AngleVectors(ps->viewangles, forward, NULL, NULL);

	vec3_t end;
	VectorMA(current_pos, 8192.0f, forward, end);

	trace_t trace;
	ai->Trace(&trace, current_pos, NULL, NULL, end, ps->clientNum, CONTENTS_SOLID);

	VectorCopy(trace.endpos, crosshair_pos);

	vec3_t to_crosshair;
	VectorSubtract(crosshair_pos, current_pos, to_crosshair);
	float dist = VectorLength(to_crosshair);

	if (dist > 0) {
		vec3_t unit_direction;
		VectorScale(to_crosshair, 1.0f / dist, unit_direction);
		VectorMA(crosshair_pos, added_distance, unit_direction, new_pos);
	} else {
		VectorMA(current_pos, added_distance, forward, new_pos);
	}

	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %f %f %f %f\n", new_pos[0], new_pos[1], new_pos[2], ps->viewangles[YAW]);
	ai->SendClientCommand(cmd);
}

// CG_HelpUsOlol_f - Set password to unlock spicy commands
static void CG_HelpUsOlol_f(void) {
	if (ai->Cmd_Argc() < 2) {
		ai->Printf("Usage: helpUsOlol <value>\n");
		return;
	}

	char argv1[MAX_STRING_CHARS];
	Q_strncpyz(argv1, ai->Cmd_Argv(1), sizeof(argv1));
	int value = atoi(argv1);

	//The idea is just to reduce abuse, if someone asks for the password and they get it, means they are unlikely to abuse, same if someone just comes to check the code here, means they have earned it in my opinion.
	if (value == 69) {
		helpUsOlolUnlocked = qtrue;
		ai->Cvar_Get("olol_helpUsOlolUnlocked", "69", 0, "Olol unlock status");
		ai->Printf("^2Commands unlocked!\n");
	} else {
		ai->Printf("^1Invalid value. Password not set.\n");
	}
}

// CG_TeleMark_f - Set a telemark at current position or specified coordinates
static void CG_TeleMark_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	if (ai->Cmd_Argc() == 1) {
		// No arguments - use current position
		customTelemarkX = ps->origin[0];
		customTelemarkY = ps->origin[1];
		customTelemarkZ = ps->origin[2];
		customTelemarkYaw = ps->viewangles[YAW];
	}
	else if (ai->Cmd_Argc() == 5) {
		// Four arguments - use specified coordinates
		customTelemarkX = atof(ai->Cmd_Argv(1));
		customTelemarkY = atof(ai->Cmd_Argv(2));
		customTelemarkZ = atof(ai->Cmd_Argv(3));
		customTelemarkYaw = atof(ai->Cmd_Argv(4));
	}
	else {
		ai->Printf("Usage: teleMark [x y z yaw]\n");
		return;
	}
	ai->Printf("Custom telemark set (%.2f %.2f %.2f) : %.2f\n", customTelemarkX, customTelemarkY, customTelemarkZ, customTelemarkYaw);
}

// CG_TeleTargetToMark_f - Teleport the target under crosshair to the telemark
static void CG_TeleTargetToMark_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	// Check if custom telemark is set
	if (!customTelemarkX && !customTelemarkY && !customTelemarkZ && !customTelemarkYaw) {
		return; // Silently fail if no telemark is set
	}

	int targetNum = ai->CrosshairPlayer();
	if (targetNum == -1) {
		return; // Silently fail if no target under crosshair
	}

	// Teleport the target to the custom telemark position
	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %i %f %f %f %f\n", targetNum, customTelemarkX, customTelemarkY, customTelemarkZ, customTelemarkYaw);
	ai->SendClientCommand(cmd);
}

// CG_TeleSelfToMark_f - Teleport yourself to the telemark
static void CG_TeleSelfToMark_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	if (!customTelemarkX && !customTelemarkY && !customTelemarkZ && !customTelemarkYaw) {
		ai->Printf("No custom telemark set!\n");
		return;
	}

	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %f %f %f %f\n", customTelemarkX, customTelemarkY, customTelemarkZ, customTelemarkYaw);
	ai->SendClientCommand(cmd);
}

// CG_TeleToMark_f - Teleport to a player's position (by name or number)
static void CG_TeleToMark_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	if (ai->Cmd_Argc() < 2) {
		ai->Printf("Usage: teleToMark <player name or number>\n");
		return;
	}

	char playerArg[MAX_STRING_CHARS];
	Q_strncpyz(playerArg, ai->Cmd_Argv(1), sizeof(playerArg));

	int targetNum = ai->ClientNumberFromString(playerArg);
	if (targetNum == -1) {
		ai->Printf("Player not found!\n");
		return;
	}

	const entityState_t *cent = (const entityState_t*)ai->GetEntityState(targetNum);
	if (!cent) {
		ai->Printf("Player entity not found!\n");
		return;
	}

	float x, y, z, yaw;
	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	int myNum = ps ? ps->clientNum : -1;

	if (targetNum == myNum) {
		x = ps->origin[0];
		y = ps->origin[1];
		z = ps->origin[2];
		yaw = ps->viewangles[YAW];
	} else {
		x = cent->pos.trBase[0];
		y = cent->pos.trBase[1];
		z = cent->pos.trBase[2];
		yaw = cent->apos.trBase[YAW];
	}

	char cmd[256];
	Com_sprintf(cmd, sizeof(cmd), "amtele %f %f %f %f\n", x, y, z, yaw);
	ai->SendClientCommand(cmd);
}

// CG_TeleRespawnMark_f - Set a respawn telemark at current position
static void CG_TeleRespawnMark_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	const playerState_t* ps = (const playerState_t*)ai->GetPredictedPlayerState();
	if (!ps) {
		return;
	}

	respawnTelemarkX = ps->origin[0];
	respawnTelemarkY = ps->origin[1];
	respawnTelemarkZ = ps->origin[2];
	respawnTelemarkYaw = ps->viewangles[YAW];
	useRespawnTelemark = qtrue;

	ai->Printf("Respawn telemark set (%.2f %.2f %.2f) : %.2f\n", respawnTelemarkX, respawnTelemarkY, respawnTelemarkZ, respawnTelemarkYaw);
}

// CG_TeleRespawnMarkClear_f - Clear the respawn telemark
static void CG_TeleRespawnMarkClear_f(void) {
	if (!helpUsOlolUnlocked) {
		ai->Printf("^1Error: To avoid abuse, a password must be set. (ask Olol)\n");
		return;
	}

	useRespawnTelemark = qfalse;
	respawnTelemarkX = 0;
	respawnTelemarkY = 0;
	respawnTelemarkZ = 0;
	respawnTelemarkYaw = 0;

	ai->Printf("Respawn telemark cleared.\n");
}

// CG_Olol_Info_f - Display help message for all Olol commands
static void CG_Olol_Info_f(void) {
	ai->Printf("^3=== Olol Commands Info ===\n");
	ai->Printf("^5To unlock locked commands, use: ^2helpUsOlol <value>\n\n");

	ai->Printf("^6--- Locked Commands (Password Required) ---\n");
	ai->Printf("^1teleFrag^7 - Teleport to a target player's predicted position (accounts for movement)\n");
	ai->Printf("^1teleFragSelf^7 - Teleport target player to your position\n");
	ai->Printf("^1teleCrosshair^7 - Teleport to where your crosshair is pointing\n");
	ai->Printf("^1teleToCrosshairWithDist^7 - Teleport to crosshair with optional distance offset\n");
	ai->Printf("^1get^7 - Teleport a target player in front of you (with optional offset)\n");
	ai->Printf("^1bring^7 - Teleport the player under your crosshair to your position\n");
	ai->Printf("^1teleport^7 - Teleport with X/Y/Z offset from current position or target player by name/number\n");
	ai->Printf("^1teleportGun^7 - Teleport the player under your crosshair with X/Y/Z offset\n");
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

	// Add all the Olol commands
	ai->Cmd_AddCommand( "get", CG_TeleTargetPlayer_f, "Teleport a target player in front of you" );
	ai->Cmd_AddCommand( "teleFrag", CG_TeleFrag_f, "Teleport to a target player's predicted position" );
	ai->Cmd_AddCommand( "teleFragSelf", CG_TeleFragSelf_f, "Teleport target player to your position" );
	ai->Cmd_AddCommand( "teleCrosshair", CG_TeleCrosshair_f, "Teleport to where your crosshair is pointing" );
	ai->Cmd_AddCommand( "teleSafeCrosshair", CG_TeleSafeCrosshair_f, "Teleport to crosshair position, avoiding players" );
	ai->Cmd_AddCommand( "bring", CG_TeleCrosshairToMe_f, "Teleport the player under your crosshair to your position" );
	ai->Cmd_AddCommand( "teleport", CG_PTele_Offset_f, "Teleport with X/Y/Z offset from current position or target player by name/number" );
	ai->Cmd_AddCommand( "teleportGun", CG_TeleportGun_f, "Teleport the player under your crosshair with X/Y/Z offset" );
	ai->Cmd_AddCommand( "teleToCrosshairWithDist", CG_TeleportToCrosshairWithDistance_f, "Teleport to crosshair with optional distance offset" );
	ai->Cmd_AddCommand( "helpUsOlol", CG_HelpUsOlol_f, "Set password to unlock spicy commands" );
	ai->Cmd_AddCommand( "teleMark", CG_TeleMark_f, "Set a telemark at current position or specified coordinates" );
	ai->Cmd_AddCommand( "teleTargetToMark", CG_TeleTargetToMark_f, "Teleport the target under crosshair to the telemark" );
	ai->Cmd_AddCommand( "teleSelfToMark", CG_TeleSelfToMark_f, "Teleport yourself to the telemark" );
	ai->Cmd_AddCommand( "teleToMark", CG_TeleToMark_f, "Teleport to a player's position" );
	ai->Cmd_AddCommand( "teleRespawnMark", CG_TeleRespawnMark_f, "Set a respawn telemark at current position" );
	ai->Cmd_AddCommand( "teleRespawnMarkClear", CG_TeleRespawnMarkClear_f, "Clear the respawn telemark" );
	ai->Cmd_AddCommand( "Olol_Info", CG_Olol_Info_f, "Display information about Olol addon commands" );

	// Register cvars
	ai->Cvar_Get( "cg_autoHeal", "0", 0, "Automatically heal when health drops below a threshold" );
	ai->Cvar_Get( "cg_instaKillOnDamage", "0", 0, "Instantly kill when taking any damage" );
	ai->Cvar_Get( "olol_helpUsOlolUnlocked", "0", 0, "Olol unlock status" );

	// Initialize unlock status from cvar
	const char* unlockValue = ai->Cvar_VariableString("olol_helpUsOlolUnlocked");
	if (unlockValue && atoi(unlockValue) == 69) {
		helpUsOlolUnlocked = qtrue;
	}

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
	ai->Cmd_RemoveCommand( "get" );
	ai->Cmd_RemoveCommand( "teleFrag" );
	ai->Cmd_RemoveCommand( "teleFragSelf" );
	ai->Cmd_RemoveCommand( "teleCrosshair" );
	ai->Cmd_RemoveCommand( "teleSafeCrosshair" );
	ai->Cmd_RemoveCommand( "bring" );
	ai->Cmd_RemoveCommand( "teleport" );
	ai->Cmd_RemoveCommand( "teleportGun" );
	ai->Cmd_RemoveCommand( "teleToCrosshairWithDist" );
	ai->Cmd_RemoveCommand( "helpUsOlol" );
	ai->Cmd_RemoveCommand( "teleMark" );
	ai->Cmd_RemoveCommand( "teleTargetToMark" );
	ai->Cmd_RemoveCommand( "teleSelfToMark" );
	ai->Cmd_RemoveCommand( "teleToMark" );
	ai->Cmd_RemoveCommand( "teleRespawnMark" );
	ai->Cmd_RemoveCommand( "teleRespawnMarkClear" );
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
	if ( apiVersion < 2 ) {
		import->Printf( "Olol addon: API version too old (%d < 2), extended features not available\n", apiVersion );
		// Still allow loading with basic functionality
	} else if ( apiVersion > ADDON_API_VERSION ) {
		import->Printf( "Olol addon: API version too new (%d > %d)\n", apiVersion, ADDON_API_VERSION );
		return NULL;
	}

	ai = import;

	return &ae;
}

#ifdef __cplusplus
}
#endif