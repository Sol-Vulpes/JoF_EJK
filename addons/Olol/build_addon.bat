@echo off
REM Build script for Olol addon

REM Set up paths
set ROOT_DIR=..\..\..
set CODEMP_DIR=%ROOT_DIR%\codemp
set SHARED_DIR=%ROOT_DIR%\shared

REM Compiler flags
set CL_FLAGS=/nologo /MT /W3 /O2 /D "WIN32" /D "_WINDOWS" /D "USE_OPENAL=1" /D "FINAL_BUILD=1" /D "Q3_LITTLE_ENDIAN=1"

REM Include paths
set INCLUDE_PATHS=/I "%CODEMP_DIR%" /I "%SHARED_DIR%"

REM Source files
set SOURCES=..\addon_olol.cpp

REM Output
set OUTPUT=Olol.dll

echo Building Olol addon...
cl %CL_FLAGS% %INCLUDE_PATHS% %SOURCES% /link /DLL /OUT:%OUTPUT%

if %ERRORLEVEL% EQU 0 (
    echo Build successful! Copying DLL to addons directory...
    copy %OUTPUT% ..\..
) else (
    echo Build failed!
)

pause