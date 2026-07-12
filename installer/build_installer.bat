@echo off
setlocal
rem ===========================================================================
rem  Builds install.exe (the noob-friendly JoF installer) with MSVC.
rem
rem  Run this from a "Developer Command Prompt for VS" (so cl.exe and rc.exe
rem  are on PATH), or just double-click it - it will try to locate and load the
rem  Visual Studio developer environment automatically via vswhere.
rem
rem  Output: installer\install.exe
rem ===========================================================================

cd /d "%~dp0"

rem --- Make sure the MSVC toolchain is available -------------------------------
where cl >nul 2>nul
if errorlevel 1 (
	echo Visual Studio compiler not found on PATH - trying to locate it...
	set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
	if not exist "%VSWHERE%" (
		echo Could not find vswhere.exe. Please run this from a
		echo "Developer Command Prompt for VS" instead.
		pause
		exit /b 1
	)
	for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
	if not defined VSPATH (
		echo No Visual C++ toolset found. Install the "Desktop development with C++" workload.
		pause
		exit /b 1
	)
	call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" || (echo Failed to init MSVC env & pause & exit /b 1)
)

rem --- Compile the resources (icon + manifest + version) -----------------------
rc /nologo /fo install.res install.rc || (echo rc failed & pause & exit /b 1)

rem --- Compile + link ----------------------------------------------------------
rem  /O1  = optimize for size, tiny exe
rem  /GS- = drop stack-cookie runtime dependency so the exe is fully standalone
cl /nologo /O1 /GS- /W3 install.c install.res /Fe:install.exe ^
	/link /SUBSYSTEM:WINDOWS user32.lib shell32.lib ole32.lib shlwapi.lib ^
	|| (echo cl failed & pause & exit /b 1)

del /q install.obj install.res 2>nul

echo.
echo Built installer\install.exe
endlocal
