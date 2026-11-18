@echo off
rem The release zip: the program only (Unchained.exe, Unchained Configuration.exe, the bundled compiler and
rem recompiler, the Visual C++ runtime), an empty assets folder, README, LICENSE and the third party notices. No game
rem files and no game code: the first start builds the game code on the player's PC.
setlocal
set "GAME_EXE=Unchained"
cd /d "%~dp0"
set "OUT=out\build\release"
rem the version from src\common\version.h, for the zip's name
for /f tokens^=2^ delims^=^" %%v in ('findstr /c:"kUnchainedVersion =" src\common\version.h') do set "VERSION=%%v"
if "%VERSION%"=="" (echo No version in src\common\version.h & exit /b 1)
rem the Visual C++ runtime beside the executable, so a fresh Windows install needs nothing more
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -property installationPath`) do set "VSDIR=%%i"
for /d %%v in ("%VSDIR%\VC\Redist\MSVC\1*") do for /d %%c in ("%%v\x64\Microsoft.VC*.CRT") do set "VCRUNTIME=%%c"
if not exist "%VCRUNTIME%\vcruntime140.dll" (echo Visual C++ runtime not found: install Visual Studio with the C++ workload & exit /b 1)
if not exist "%OUT%\%GAME_EXE%.exe" call build.bat || exit /b 1
rem the configure step writes where the SDK is
if not exist "%OUT%\sdk_dir.txt" cmake --preset release >nul || exit /b 1
if not exist "%OUT%\sdk_dir.txt" echo %OUT%\sdk_dir.txt was not written, update the SDK. && exit /b 1
set /p SDK=<"%OUT%\sdk_dir.txt"

rem the release laid out in a folder of its own
rem the OS and CPU in the name, so a launcher or a player picks the right file
set "NAME=%GAME_EXE%-%VERSION%-Windows-x64"
set "STAGE=%OUT%\package\%NAME%"
rd /s /q "%OUT%\package" 2>nul
mkdir "%STAGE%\assets"
copy /y "%OUT%\%GAME_EXE%.exe" "%STAGE%\" >nul || exit /b 1
rem a Configuration app beside the game (the same executable, opening its settings)
copy /y "%OUT%\%GAME_EXE%.exe" "%STAGE%\%GAME_EXE% Configuration.exe" >nul
rem the compiler folder without its work folder (no game code); xbe_recomp.exe needs its own copy of the runtime
rem DLLs, Windows only looks beside the exe it starts
robocopy "%OUT%\compiler" "%STAGE%\compiler" /e /xd work /njh /njs /nfl /ndl >nul
if errorlevel 8 (echo Copying the compiler failed & exit /b 1)
del /q "%STAGE%\compiler\*.dll" 2>nul
for %%f in (msvcp140 vcruntime140) do copy /y "%VCRUNTIME%\%%f.dll" "%STAGE%\compiler\" >nul
for %%f in (msvcp140 vcruntime140 vcruntime140_1) do copy /y "%VCRUNTIME%\%%f.dll" "%STAGE%\" >nul
copy /y res\splash.png "%STAGE%\" >nul
robocopy fonts "%STAGE%\fonts" /njh /njs /nfl /ndl >nul
if errorlevel 8 (echo Copying the fonts failed & exit /b 1)
rem RecompLauncher reads this beside the exe: the game it needs, its settings, its saves
copy /y recomplauncher.json "%STAGE%\" >nul || exit /b 1
copy /y README.md "%STAGE%\" >nul
copy /y LICENSE "%STAGE%\LICENSE.txt" >nul
copy /y "%SDK%\THIRD_PARTY.txt" "%STAGE%\" >nul
echo Put your own copy of Whiplash here (its .iso or extracted files), or pick it in Unchained Configuration.> "%STAGE%\assets\README.txt"

rem zipped with 7-Zip when it is installed, else with Windows' own tar
del /q "%NAME%.zip" 2>nul
where 7z >nul 2>nul
if not errorlevel 1 (
    pushd "%OUT%\package" && 7z a -tzip "%~dp0%NAME%.zip" "%NAME%" >nul && popd || exit /b 1
) else (
    rem Windows' own tar, which writes a zip for -a (another tar on the PATH, like Git's, writes a tar archive)
    "%SystemRoot%\System32\tar.exe" -a -c -f "%NAME%.zip" -C "%OUT%\package" "%NAME%" || exit /b 1
)
echo %NAME%.zip

rem the single exe installer (tools\installer), when NSIS is installed: art drawn by tools\installer\art.py
set "MAKENSIS="
where makensis >nul 2>nul && set "MAKENSIS=makensis"
if not defined MAKENSIS if exist "%ProgramFiles(x86)%\NSIS\makensis.exe" set "MAKENSIS=%ProgramFiles(x86)%\NSIS\makensis.exe"
if not defined MAKENSIS (echo NSIS not installed: no installer, the zip only & exit /b 0)
py -3 tools\installer\art.py "%OUT%\installer-art" "%VERSION%" >nul || exit /b 1
del /q "%NAME%-Setup.exe" 2>nul
"%MAKENSIS%" /V2 /DVERSION=%VERSION% "/DDIST=%~dp0%STAGE%" "/DART=%~dp0%OUT%\installer-art" "/DOUTFILE=%~dp0%NAME%-Setup.exe" tools\installer\unchained.nsi || exit /b 1
echo %NAME%-Setup.exe
