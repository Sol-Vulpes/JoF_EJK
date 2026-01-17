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

/*
============
Addon_Init
============
*/
static qboolean Addon_Init( void ) {
	// Add our test command
	ai->Cmd_AddCommand( "test_olol", Cmd_TestOlol_f, "Test command for Olol addon" );

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