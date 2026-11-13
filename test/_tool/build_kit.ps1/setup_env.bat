@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul
title EduDoom-Ass v2.1.0 Setup

:: ==========================================================================
:: [WARNING] Please save this file using ANSI encoding (or UTF-8 without BOM)
:: to prevent Windows CMD from swallowing the first character of each line.
:: ==========================================================================

:: ==========================================================================
:: KIT CONFIGURATION & PATHS
:: ==========================================================================
set KIT_VERSION=2.1.0
set KIT_URL=https://github.com/yourusername/edudoom-ass-kit/releases/download/v2.1.0/kit-v2.1.0.zip
set KIT_SHA256=8db24dd87446d31fa1023c6a99c7c3f87813f89574fc55c145d3a771001f4f

set "PROJECT_DIR=%~dp0"
set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"
set "CACHE_DIR=%PROJECT_DIR%\.download_cache"
set "LOCAL_KIT=%PROJECT_DIR%\kit-v%KIT_VERSION%.zip"

if not exist "%CACHE_DIR%" mkdir "%CACHE_DIR%"
cd /d "%PROJECT_DIR%"

:: ==========================================================================
:: STEP 1: DOWNLOAD / COPY KIT & VERIFY SHA256
:: ==========================================================================
echo =======================================================
echo [STEP 1] Locating and verifying Kit package...
echo =======================================================

:: 1.1 Get the Kit - Local first, then Download
if exist "%LOCAL_KIT%" (
    echo Found local kit file. Copying to cache...
    copy /Y "%LOCAL_KIT%" "%CACHE_DIR%\kit-v%KIT_VERSION%.zip" >nul
) else if not exist "%CACHE_DIR%\kit-v%KIT_VERSION%.zip" (
    echo Kit not found in cache. Downloading from URL...
    powershell -Command "Invoke-WebRequest -Uri '%KIT_URL%' -OutFile '%CACHE_DIR%\kit-v%KIT_VERSION%.zip'"
)

:: 1.2 SHA256 Verification Loop
:CHECK_SHA
echo.
echo Verifying SHA256...
set "ACTUAL_HASH="
for /f "tokens=*" %%H in ('powershell -Command "(Get-FileHash -Path '%CACHE_DIR%\kit-v%KIT_VERSION%.zip' -Algorithm SHA256).Hash.ToLower()"') do set "ACTUAL_HASH=%%H"

if "%ACTUAL_HASH%"=="%KIT_SHA256%" goto :SHA_OK

echo =======================================================
echo [ERROR] SHA256 MISMATCH! FILE MAY BE CORRUPTED OR TAMPERED.
echo.
echo [INSTRUCTIONS TO FIX]
echo 1. Open PowerShell and run this exact command to check the actual file hash:
echo    ^(Get-FileHash -Path "%CACHE_DIR%\kit-v%KIT_VERSION%.zip" -Algorithm SHA256^).Hash.ToLower^(^)
echo 2. If you are a developer and the file is safe, paste the correct hash below.
echo 3. If you are a normal user, DO NOT paste. Delete the zip file and re-download.
echo =======================================================
set /p KIT_SHA256="Enter correct SHA256 (or press Ctrl+C to exit): "
goto CHECK_SHA

:SHA_OK
echo [OK] SHA256 matches!

:: 1.3 Extract Kit (Safe Mode)
if exist "%CACHE_DIR%\kit-v%KIT_VERSION%\manifest.json" (
    echo [SKIP] Kit source already exists. Skipping extraction to protect your edits.
) else (
    echo Extracting Kit...
    if exist "%CACHE_DIR%\kit-v%KIT_VERSION%" rmdir /S /Q "%CACHE_DIR%\kit-v%KIT_VERSION%"
    mkdir "%CACHE_DIR%\kit-v%KIT_VERSION%"
    tar -xf "%CACHE_DIR%\kit-v%KIT_VERSION%.zip" -C "%CACHE_DIR%\kit-v%KIT_VERSION%"
    echo [OK] Kit extracted.
)

:: ==========================================================================
:: STEP 2: EXTRACT DEPENDENCIES - SAFE MODE
:: ==========================================================================
echo.
echo =======================================================
echo [STEP 2] Extracting 64-bit dependencies...
echo =======================================================

:: 2.1 WinLibs - .7z
call :EXTRACT_WINLIBS

:: 2.2 SDL2 - .tar.gz (Safe Mode)
if exist "third_party\SDL2\lib" (
    echo [SKIP] SDL2 already exists.
) else if exist "%CACHE_DIR%\SDL2-devel-2.0.20-mingw.tar.gz" (
    echo Extracting SDL2...
    if exist "third_party\SDL2" rmdir /S /Q "third_party\SDL2"
    mkdir "third_party\SDL2"
    tar -xf "%CACHE_DIR%\SDL2-devel-2.0.20-mingw.tar.gz" -C "third_party\SDL2"
    if exist "third_party\SDL2\SDL2-2.0.20\x86_64-w64-mingw32" (
        xcopy /E /I /Y "third_party\SDL2\SDL2-2.0.20\x86_64-w64-mingw32\*" "third_party\SDL2\" >nul
        rmdir /S /Q "third_party\SDL2\SDL2-2.0.20"
    )
    echo [OK] SDL2.
)

:: 2.3 SDL2_mixer - .zip (Safe Mode)
if exist "third_party\SDL2_mixer\lib" (
    echo [SKIP] SDL2_mixer already exists.
) else if exist "%CACHE_DIR%\SDL2_mixer-devel-2.8.1-mingw.zip" (
    echo Extracting SDL2_mixer...
    if exist "third_party\SDL2_mixer" rmdir /S /Q "third_party\SDL2_mixer"
    mkdir "third_party\SDL2_mixer"
    tar -xf "%CACHE_DIR%\SDL2_mixer-devel-2.8.1-mingw.zip" -C "third_party\SDL2_mixer"
    if exist "third_party\SDL2_mixer\x86_64-w64-mingw32" (
        xcopy /E /I /Y "third_party\SDL2_mixer\x86_64-w64-mingw32\*" "third_party\SDL2_mixer\" >nul
        rmdir /S /Q "third_party\SDL2_mixer\x86_64-w64-mingw32"
    ) else if exist "third_party\SDL2_mixer\SDL2_mixer-2.8.1\x86_64-w64-mingw32" (
        xcopy /E /I /Y "third_party\SDL2_mixer\SDL2_mixer-2.8.1\x86_64-w64-mingw32\*" "third_party\SDL2_mixer\" >nul
        rmdir /S /Q "third_party\SDL2_mixer\SDL2_mixer-2.8.1"
    )
    echo [OK] SDL2_mixer.
)

:: 2.4 Doomgeneric Source (Safe Mode)
if exist "%CACHE_DIR%\doomgeneric-master.zip" (
    if not exist "doomgeneric-master\doomgeneric\doomgeneric.c" (
        echo Extracting Doomgeneric Source...
        tar -xf "%CACHE_DIR%\doomgeneric-master.zip" -C "."
        echo [OK] Doomgeneric source extracted.
    ) else (
        echo [SKIP] Doomgeneric source already exists. Skipping extraction to protect your edits.
    )
)

:: 2.5 DOOM1.WAD
if exist "%CACHE_DIR%\DOOM1.WAD" (
    echo Copying WAD file...
    if not exist "wads" mkdir "wads"
    copy /Y "%CACHE_DIR%\DOOM1.WAD" "wads\DOOM1.WAD" >nul
    echo [OK] WAD.
)
goto :AFTER_EXTRACT

:: ==========================================================================
:: SUBROUTINE: EXTRACT WINLIBS (SAFE MODE & ERROR CHECKING)
:: ==========================================================================
:EXTRACT_WINLIBS
if exist "mingw64\bin\gcc.exe" (
    echo [SKIP] WinLibs Toolchain already exists.
    exit /b
)

set "WINLIBS_FILE="
for %%F in ("%CACHE_DIR%\winlibs-*.7z") do set "WINLIBS_FILE=%%~fF"
if not defined WINLIBS_FILE exit /b

echo Extracting Toolchain WinLibs...
if exist "mingw64" rmdir /S /Q "mingw64"
mkdir "mingw64"

set "SEVEN_ZIP=C:\Program Files\7-Zip\7z.exe"
if exist "%ProgramFiles%\7-Zip\7z.exe" set "SEVEN_ZIP=%ProgramFiles%\7-Zip\7z.exe"
if exist "%ProgramFiles(x86)%\7-Zip\7z.exe" set "SEVEN_ZIP=%ProgramFiles(x86)%\7-Zip\7z.exe"

if not exist "!SEVEN_ZIP!" goto :USE_TAR_FALLBACK

"!SEVEN_ZIP!" x "!WINLIBS_FILE!" -o"mingw64" -y >nul
if errorlevel 1 (
    echo [WARNING] Some files in WinLibs could not be extracted. They might be in use by another program.
)
goto :CLEANUP_WINLIBS

:USE_TAR_FALLBACK
echo [WARNING] 7-Zip not found. Trying tar...
tar -xf "!WINLIBS_FILE!" -C "mingw64" >nul 2>&1

:CLEANUP_WINLIBS
if exist "mingw64\mingw64\bin\gcc.exe" (
    xcopy /E /I /Y "mingw64\mingw64\*" "mingw64\" >nul
    rmdir /S /Q "mingw64\mingw64"
)
for /d %%D in ("mingw64\winlibs-*") do (
    if exist "%%D\mingw64\bin\gcc.exe" (
        xcopy /E /I /Y "%%D\mingw64\*" "mingw64\" >nul
        rmdir /S /Q "%%D"
    )
)
echo [OK] WinLibs.
exit /b

:AFTER_EXTRACT
:: ==========================================================================
:: STEP 3: APPLY OVERLAY - SAFE OVERLAY
:: ==========================================================================
echo.
echo =======================================================
echo [STEP 3] Applying code overlay and Makefile...
echo =======================================================

if not exist "doomgeneric-master\doomgeneric" mkdir "doomgeneric-master\doomgeneric"
if not exist "Custom build Makefile" mkdir "Custom build Makefile"

:: 3.1 Safe Overlay for doomgeneric_sdl.c
if not exist "doomgeneric-master\doomgeneric\doomgeneric_sdl.c" (
    copy /Y "%CACHE_DIR%\kit-v%KIT_VERSION%\src\doomgeneric_sdl.c" "doomgeneric-master\doomgeneric\doomgeneric_sdl.c" >nul
    echo [OK] doomgeneric_sdl.c applied.
) else (
    copy /Y "%CACHE_DIR%\kit-v%KIT_VERSION%\src\doomgeneric_sdl.c" "doomgeneric-master\doomgeneric\doomgeneric_sdl_new.c" >nul
    echo [WARNING] doomgeneric_sdl.c already exists.
    echo           New version saved as doomgeneric_sdl_new.c to protect your edits.
)

:: 3.2 Safe Overlay for Makefile.win
if not exist "Custom build Makefile\Makefile.win" (
    copy /Y "%CACHE_DIR%\kit-v%KIT_VERSION%\make\Makefile.win" "Custom build Makefile\Makefile.win" >nul
    echo [OK] Makefile.win applied.
) else (
    copy /Y "%CACHE_DIR%\kit-v%KIT_VERSION%\make\Makefile.win" "Custom build Makefile\Makefile_new.win" >nul
    echo [WARNING] Makefile.win already exists.
    echo           New version saved as Makefile_new.win to protect your edits.
)

:: ==========================================================================
:: STEP 4: RUN POWERSHELL SCRIPTS - DOCS & TELEMETRY
:: ==========================================================================
echo.
echo =======================================================
echo [STEP 4] Updating documentation and telemetry...
echo =======================================================

set "KIT_DIR=%CACHE_DIR%\kit-v%KIT_VERSION%"

powershell -NoProfile -ExecutionPolicy Bypass -File "%KIT_DIR%\scripts\install_docs.ps1"

:: ==========================================================================
:: STEP 5: GENERATE BUILD_INFO.TXT & SUMMARY
:: ==========================================================================
echo.
echo Generating build_info.txt...

(
echo =======================================================
echo EDU DOOM ASS - BUILD INFO
echo =======================================================
echo Date/Time  : %date% %time%
echo Version    : %KIT_VERSION%
echo Project Dir: %PROJECT_DIR%
echo.
echo [Created/Extracted Directories]
echo - .download_cache\kit-v%KIT_VERSION%\ - Kit source
echo - doomgeneric-master\ - Doom source code
echo - third_party\ - SDL2 and SDL2_mixer 64-bit
echo - mingw64\ - WinLibs Toolchain 64-bit
echo - wads\ - DOOM1.WAD
echo - assets\document\ - Markdown Documentation
echo - Custom build Makefile\ - Makefile.win
echo.
echo [Next Steps]
echo 1. Open Geany IDE.
echo 2. Press F9 to Build.
echo 3. Press F5 to Run.
echo =======================================================
) > "%PROJECT_DIR%\build_info.txt"

echo.
echo =======================================================
echo [SUMMARY] Setup completed successfully!
echo =======================================================
echo - Extracted Kit to  : .download_cache\kit-v%KIT_VERSION%\
echo - Applied Source Code: doomgeneric-master\doomgeneric\ - Safe Mode
echo - Applied Makefile   : Custom build Makefile\ - Safe Mode
echo - Generated Docs     : assets\document\
echo - Build log saved at : build_info.txt
echo =======================================================
type "%PROJECT_DIR%\build_info.txt"
pause