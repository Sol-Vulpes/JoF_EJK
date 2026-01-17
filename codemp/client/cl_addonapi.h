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

#ifndef CL_ADDONAPI_H
#define CL_ADDONAPI_H

#include "../qcommon/qcommon.h"

#define ADDON_API_VERSION 1

//
// these are the functions exported by the addon module
//
typedef struct addonexport_s {
	// called before the library is unloaded
	void			(*Shutdown)							( qboolean restarting );

	// called when the addon is loaded
	qboolean		(*Init)								( void );
} addonexport_t;

//
// these are the functions imported by the addon module
//
typedef struct addonimport_s {
	void			(QDECL *Printf)						( const char *msg, ... ) __attribute__ ((format (printf, 1, 2)));
	void			(QDECL *Error)						( int level, const char *error, ... ) NORETURN_PTR __attribute__ ((format (printf, 2, 3)));

	// console commands
	void			(*Cmd_AddCommand)					( const char *cmd_name, xcommand_t function, const char *cmd_desc );
	void			(*Cmd_RemoveCommand)				( const char *cmd_name );

	// cvars
	cvar_t *		(*Cvar_Get)							( const char *var_name, const char *value, uint32_t flags, const char *var_desc );
	char *			(*Cvar_VariableString)				( const char *var_name );
	float			(*Cvar_VariableValue)				( const char *var_name );

	// file system
	long			(*FS_ReadFile)						( const char *qpath, void **buffer );
	void			(*FS_FreeFile)						( void *buffer );
	qboolean		(*FS_FileExists)					( const char *file );

	// memory management
	void *			(*Z_Malloc)							( int iSize, memtag_t eTag, qboolean bZeroit /*= qfalse*/, int iAlign /*= 4*/);
	void			(*Z_Free)							( void *ptr );
} addonimport_t;

// this is the only function actually exported at the linker level
typedef	addonexport_t* (QDECL *GetAddonAPI_t) (int apiVersion, addonimport_t *rimp);

#endif