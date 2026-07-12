/*
==============================================================================
JoF (EternalJK) - simple Windows installer for non-technical users.

What it does:
  * Finds the "GameData" folder that ships next to this install.exe (inside the
    release zip) and merges its contents into the user's own Jedi Academy
    "GameData" folder.
  * Auto-detects a Jedi Academy install (Steam - including extra Steam library
    folders -, GOG and retail/LucasArts), pre-selecting it in the folder picker,
    but the user can always browse to any folder (not everyone owns it on Steam).

It is deliberately tiny: no third-party dependencies, just Win32 + the shell.
Built with MSVC (see build_installer.bat and the CI step in .github/workflows).

Layout it expects inside the extracted release zip:

    <this folder>\
        install.exe        <- you are here
        GameData\
            eternaljk.x86_64.exe
            rd-*.dll ...
            EternalJK\ ...
            base\ ...

Copyright (C) JoF contributors. GPLv2, like the rest of the project.
==============================================================================
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdio.h>

#define APP_TITLE L"JoF Installer"

/* ------------------------------------------------------------------------- */
/* Small helpers                                                             */
/* ------------------------------------------------------------------------- */

/* Directory that contains this running executable. */
static void GetExeDir( wchar_t *out, size_t count ) {
	GetModuleFileNameW( NULL, out, (DWORD)count );
	PathRemoveFileSpecW( out );
}

/* TRUE if the path exists and is a directory. */
static BOOL DirExists( const wchar_t *path ) {
	DWORD attr = GetFileAttributesW( path );
	return ( attr != INVALID_FILE_ATTRIBUTES ) && ( attr & FILE_ATTRIBUTE_DIRECTORY );
}

/* A folder "looks like" a Jedi Academy GameData folder if it has a base\ subdir. */
static BOOL LooksLikeGameData( const wchar_t *path ) {
	wchar_t base[MAX_PATH];
	PathCombineW( base, path, L"base" );
	return DirExists( base );
}

/* Append a candidate to the list if it exists and looks like a real GameData. */
static BOOL FirstValid( const wchar_t *candidate, wchar_t *out ) {
	if ( DirExists( candidate ) && LooksLikeGameData( candidate ) ) {
		wcscpy_s( out, MAX_PATH, candidate );
		return TRUE;
	}
	return FALSE;
}

/* ------------------------------------------------------------------------- */
/* Steam library-folders parsing                                             */
/* ------------------------------------------------------------------------- */
/*
 * Steam records every library location in
 *   <Steam>\steamapps\libraryfolders.vdf
 * as lines like:      "path"    "D:\\SteamLibrary"
 * We scan for each "path" value, unescape the doubled backslashes, and test
 * <path>\steamapps\common\Jedi Academy\GameData.
 */
static BOOL DetectFromSteamLibraries( const wchar_t *steamRoot, wchar_t *out ) {
	wchar_t vdf[MAX_PATH];
	PathCombineW( vdf, steamRoot, L"steamapps\\libraryfolders.vdf" );

	HANDLE h = CreateFileW( vdf, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( h == INVALID_HANDLE_VALUE ) {
		return FALSE;
	}

	DWORD size = GetFileSize( h, NULL );
	if ( size == INVALID_FILE_SIZE || size == 0 || size > 1024 * 1024 ) {
		CloseHandle( h );
		return FALSE;
	}

	char *buf = (char *)malloc( size + 1 );
	if ( !buf ) {
		CloseHandle( h );
		return FALSE;
	}

	DWORD read = 0;
	ReadFile( h, buf, size, &read, NULL );
	buf[read] = '\0';
	CloseHandle( h );

	BOOL found = FALSE;
	const char *p = buf;
	while ( ( p = strstr( p, "\"path\"" ) ) != NULL ) {
		p += 6;
		/* skip to the opening quote of the value */
		while ( *p && *p != '"' ) p++;
		if ( *p != '"' ) break;
		p++;

		char value[MAX_PATH];
		int vi = 0;
		while ( *p && *p != '"' && vi < MAX_PATH - 1 ) {
			if ( *p == '\\' && *( p + 1 ) == '\\' ) { /* unescape \\ -> \ */
				value[vi++] = '\\';
				p += 2;
			} else {
				value[vi++] = *p++;
			}
		}
		value[vi] = '\0';

		/* Build <value>\steamapps\common\Jedi Academy\GameData and test it. */
		wchar_t wvalue[MAX_PATH];
		MultiByteToWideChar( CP_UTF8, 0, value, -1, wvalue, MAX_PATH );

		wchar_t candidate[MAX_PATH];
		PathCombineW( candidate, wvalue, L"steamapps\\common\\Jedi Academy\\GameData" );
		if ( FirstValid( candidate, out ) ) {
			found = TRUE;
			break;
		}
	}

	free( buf );
	return found;
}

/* Try every common location and return the first real GameData folder. */
static BOOL DetectGameData( wchar_t *out ) {
	wchar_t candidate[MAX_PATH];
	wchar_t pf86[MAX_PATH]  = L"";
	wchar_t pf[MAX_PATH]    = L"";

	GetEnvironmentVariableW( L"ProgramFiles(x86)", pf86, MAX_PATH );
	GetEnvironmentVariableW( L"ProgramFiles",      pf,   MAX_PATH );

	/* 1. Default Steam install, then any other Steam library folders. */
	if ( pf86[0] ) {
		wchar_t steamRoot[MAX_PATH];
		PathCombineW( steamRoot, pf86, L"Steam" );

		PathCombineW( candidate, steamRoot, L"steamapps\\common\\Jedi Academy\\GameData" );
		if ( FirstValid( candidate, out ) ) return TRUE;

		if ( DetectFromSteamLibraries( steamRoot, out ) ) return TRUE;
	}

	/* 2. GOG default location. */
	if ( FirstValid( L"C:\\GOG Games\\Star Wars Jedi Knight - Jedi Academy\\GameData", out ) )
		return TRUE;

	/* 3. Retail / LucasArts CD install. */
	if ( pf86[0] ) {
		PathCombineW( candidate, pf86, L"LucasArts\\Star Wars Jedi Knight Jedi Academy\\GameData" );
		if ( FirstValid( candidate, out ) ) return TRUE;
	}
	if ( pf[0] ) {
		PathCombineW( candidate, pf, L"LucasArts\\Star Wars Jedi Knight Jedi Academy\\GameData" );
		if ( FirstValid( candidate, out ) ) return TRUE;
	}

	out[0] = L'\0';
	return FALSE;
}

/* ------------------------------------------------------------------------- */
/* Folder picker                                                             */
/* ------------------------------------------------------------------------- */

static int CALLBACK BrowseCallback( HWND hwnd, UINT msg, LPARAM lp, LPARAM data ) {
	if ( msg == BFFM_INITIALIZED && data ) {
		/* Pre-select the auto-detected folder. */
		SendMessageW( hwnd, BFFM_SETSELECTIONW, TRUE, data );
	}
	return 0;
}

static BOOL BrowseForGameData( const wchar_t *initial, wchar_t *out ) {
	BROWSEINFOW bi;
	ZeroMemory( &bi, sizeof( bi ) );
	bi.lpszTitle = L"Select your Jedi Academy \"GameData\" folder\n"
	               L"(where eternaljk / jamp / a 'base' folder lives)";
	bi.ulFlags   = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
	if ( initial && initial[0] ) {
		bi.lpfn   = BrowseCallback;
		bi.lParam = (LPARAM)initial;
	}

	LPITEMIDLIST pidl = SHBrowseForFolderW( &bi );
	if ( !pidl ) {
		return FALSE; /* user cancelled */
	}

	BOOL ok = SHGetPathFromIDListW( pidl, out );
	CoTaskMemFree( pidl );
	return ok;
}

/* ------------------------------------------------------------------------- */
/* Copy                                                                      */
/* ------------------------------------------------------------------------- */

/* Merge srcDir\* into dstDir using the shell (recursive, with progress UI). */
static BOOL CopyTree( const wchar_t *srcDir, const wchar_t *dstDir, HWND owner ) {
	/* SHFileOperation wants double-null-terminated lists. */
	wchar_t from[MAX_PATH + 4];
	wchar_t to[MAX_PATH + 4];
	ZeroMemory( from, sizeof( from ) );
	ZeroMemory( to,   sizeof( to ) );

	swprintf_s( from, MAX_PATH + 2, L"%s\\*", srcDir );
	wcscpy_s( to, MAX_PATH + 2, dstDir );

	SHFILEOPSTRUCTW op;
	ZeroMemory( &op, sizeof( op ) );
	op.hwnd   = owner;
	op.wFunc  = FO_COPY;
	op.pFrom  = from; /* the extra '\0' from ZeroMemory double-terminates it */
	op.pTo    = to;
	op.fFlags = FOF_NOCONFIRMATION   /* overwrite existing files silently     */
	          | FOF_NOCONFIRMMKDIR;  /* create the target tree without asking */

	int rc = SHFileOperationW( &op );
	return ( rc == 0 && !op.fAnyOperationsAborted );
}

/* ------------------------------------------------------------------------- */
/* Entry point                                                               */
/* ------------------------------------------------------------------------- */

int WINAPI wWinMain( HINSTANCE hInst, HINSTANCE hPrev, LPWSTR cmdLine, int show ) {
	(void)hInst; (void)hPrev; (void)cmdLine; (void)show;

	CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

	/* Locate the bundled GameData folder that ships next to this exe. */
	wchar_t exeDir[MAX_PATH];
	GetExeDir( exeDir, MAX_PATH );

	wchar_t srcDir[MAX_PATH];
	PathCombineW( srcDir, exeDir, L"GameData" );
	if ( !DirExists( srcDir ) ) {
		MessageBoxW( NULL,
			L"Could not find the bundled \"GameData\" folder next to this installer.\n\n"
			L"Make sure you extracted the WHOLE .zip first, then run install.exe from "
			L"inside the extracted folder (do not run it straight from the zip).",
			APP_TITLE, MB_ICONERROR | MB_OK );
		CoUninitialize();
		return 1;
	}

	/* Welcome / auto-detect. */
	wchar_t detected[MAX_PATH];
	BOOL haveGuess = DetectGameData( detected );

	wchar_t intro[1024];
	if ( haveGuess ) {
		swprintf_s( intro, 1024,
			L"This will install JoF (EternalJK) into your Jedi Academy game.\n\n"
			L"A Jedi Academy install was detected here:\n    %s\n\n"
			L"Click OK, then confirm or change that folder in the next dialog.",
			detected );
	} else {
		wcscpy_s( intro, 1024,
			L"This will install JoF (EternalJK) into your Jedi Academy game.\n\n"
			L"No Jedi Academy install was auto-detected. Click OK, then browse to your "
			L"\"GameData\" folder (the folder that contains 'base') in the next dialog." );
	}
	if ( MessageBoxW( NULL, intro, APP_TITLE, MB_ICONINFORMATION | MB_OKCANCEL ) != IDOK ) {
		CoUninitialize();
		return 0;
	}

	/* Ask the user where to install. */
	wchar_t dstDir[MAX_PATH];
	if ( !BrowseForGameData( haveGuess ? detected : NULL, dstDir ) ) {
		CoUninitialize();
		return 0; /* cancelled */
	}

	/* Sanity check: warn (but allow) if it doesn't look like GameData. */
	if ( !LooksLikeGameData( dstDir ) ) {
		wchar_t warn[1024];
		swprintf_s( warn, 1024,
			L"This folder does not contain a 'base' folder, so it may not be your "
			L"Jedi Academy \"GameData\" folder:\n\n    %s\n\nInstall here anyway?",
			dstDir );
		if ( MessageBoxW( NULL, warn, APP_TITLE, MB_ICONWARNING | MB_YESNO ) != IDYES ) {
			CoUninitialize();
			return 0;
		}
	}

	/* Do the copy. */
	if ( CopyTree( srcDir, dstDir, NULL ) ) {
		wchar_t done[1024];
		swprintf_s( done, 1024,
			L"Installation complete!\n\n"
			L"Files were installed to:\n    %s\n\n"
			L"Launch the game with the eternaljk executable in that folder.",
			dstDir );
		MessageBoxW( NULL, done, APP_TITLE, MB_ICONINFORMATION | MB_OK );
	} else {
		MessageBoxW( NULL,
			L"Some files could not be copied.\n\n"
			L"The most common cause is the game being open while installing. "
			L"Please CLOSE Jedi Academy / eternaljk and run this installer again.",
			APP_TITLE, MB_ICONERROR | MB_OK );
		CoUninitialize();
		return 1;
	}

	CoUninitialize();
	return 0;
}
