@echo off
setlocal

set "SRC=S:\Git\Not mine\JoF_EJK\build64temp\Release"
set "PK3SRC=S:\Git\Not mine\JoF_EJK\build64temp\codemp"
set "GAMEDATA=C:\Program Files (x86)\Steam\steamapps\common\Jedi Academy\GameData"
set "ETERNALJK=%GAMEDATA%\EternalJK"

echo Copying main executables/DLLs to GameData (only if newer)...
robocopy "%SRC%" "%GAMEDATA%" eternaljk.x86_64.exe rd-eternaljk_x86_64.dll rd-null_x86_64.dll eternaljkded.x86_64.exe /XO /R:1 /W:1

echo.
echo Copying EternalJK files to GameData\EternalJK (only if newer)...
robocopy "%SRC%" "%ETERNALJK%" cgamex86_64.dll jampgamex86_64.dll compact_glsl.exe uix86_64.dll /XO /R:1 /W:1

echo.
echo Copying asset pk3s to GameData\EternalJK (only if newer)...
robocopy "%PK3SRC%" "%ETERNALJK%" jofclient-assets.pk3 japro-assets.pk3 /XO /R:1 /W:1

echo.
echo Done.
pause
