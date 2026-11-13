@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul
title EduDoom-Ass v2.1.0 Setup

:: ==========================================================================
:: CẤU HÌNH KIT & ĐƯỜNG DẪN TEST
:: ==========================================================================
set KIT_VERSION=2.1.0
set KIT_URL=file:///E:/Workspace/Doom/v2.1.0-Level-1A/test/_tool/build_kit.ps1/kit-v2.1.0.zip
set KIT_SHA256=6489ca3e2fda0e83ad7536bf666592f4dd13e6a2ff62ae4ff607cb54167cb08a

:: ==========================================================================
:: BƯỚC 1: THIẾT LẬP MÔI TRƯỜNG & TẢI KIT
:: ==========================================================================
echo =======================================================
echo [BUOC 1] Dang tai goi Kit...
echo =======================================================
set "PROJECT_DIR=%~dp0"
set "PROJECT_DIR=%PROJECT_DIR:~0,-1%"
set "CACHE_DIR=%PROJECT_DIR%\.download_cache"

if not exist "%CACHE_DIR%" mkdir "%CACHE_DIR%"
cd /d "%PROJECT_DIR%"

:: 1.1 Tải kit
if not exist "%CACHE_DIR%\kit-v%KIT_VERSION%.zip" (
    echo Dang tai kit-v%KIT_VERSION%.zip...
    powershell -Command "Invoke-WebRequest -Uri '%KIT_URL%' -OutFile '%CACHE_DIR%\kit-v%KIT_VERSION%.zip'"
)

:: 1.2 Kiểm tra SHA256
echo Dang kiem tra ma bam SHA256...
for /f "tokens=*" %%H in ('powershell -Command "(Get-FileHash -Path '%CACHE_DIR%\kit-v%KIT_VERSION%.zip' -Algorithm SHA256).Hash.ToLower()"') do set "ACTUAL_HASH=%%H"
if not "%ACTUAL_HASH%"=="%KIT_SHA256%" (
    echo [LOI] SHA256 KHONG KHOP! File bi hong hoac bi chinh sua.
    pause
    exit /b 1
)
echo [OK] SHA256 khop.

:: 1.2.1 Giải nén kit
echo Dang giai nen kit...
if exist "%CACHE_DIR%\kit-v%KIT_VERSION%" rmdir /S /Q "%CACHE_DIR%\kit-v%KIT_VERSION%"
mkdir "%CACHE_DIR%\kit-v%KIT_VERSION%"
tar -xf "%CACHE_DIR%\kit-v%KIT_VERSION%.zip" -C "%CACHE_DIR%\kit-v%KIT_VERSION%"
echo [OK] Da giai nen kit.

:: 1.3 Giải nén các gói phụ thuộc
echo.
echo =======================================================
echo [BUOC 1.3] Dang giai nen cac goi phu thuoc...
echo =======================================================

:: 1. WinLibs (.7z) -> mingw64
if exist "%CACHE_DIR%\winlibs-*.7z" (
    echo Dang giai nen Toolchain WinLibs...
    if not exist "mingw64" mkdir "mingw64"
    tar -xf "%CACHE_DIR%\winlibs-*.7z" -C "mingw64" --strip-components=1
)

:: 2. SDL2 (.tar.gz) -> third_party\SDL2
if exist "%CACHE_DIR%\SDL2-devel-2.0.20-mingw.tar.gz" (
    echo Dang giai nen SDL2...
    if not exist "third_party\SDL2" mkdir "third_party\SDL2"
    tar -xf "%CACHE_DIR%\SDL2-devel-2.0.20-mingw.tar.gz" -C "third_party\SDL2" --strip-components=2
)

:: 3. SDL2_mixer (.zip) -> third_party\SDL2_mixer
if exist "%CACHE_DIR%\SDL2_mixer-devel-2.8.1-mingw.zip" (
    echo Dang giai nen SDL2_mixer...
    if not exist "third_party\SDL2_mixer" mkdir "third_party\SDL2_mixer"
    tar -xf "%CACHE_DIR%\SDL2_mixer-devel-2.8.1-mingw.zip" -C "third_party\SDL2_mixer" --strip-components=2
)

:: 4. Doomgeneric Source -> doomgeneric-master
if exist "%CACHE_DIR%\doomgeneric-master.zip" (
    echo Dang giai nen Doomgeneric Source...
    tar -xf "%CACHE_DIR%\doomgeneric-master.zip" -C "."
)

:: 5. Doom1.wad -> wads\DOOM1.WAD
if exist "%CACHE_DIR%\DOOM1.WAD" (
    echo Dang copy file WAD...
    if not exist "wads" mkdir "wads"
    copy /Y "%CACHE_DIR%\DOOM1.WAD" "wads\DOOM1.WAD" >nul
)
echo [OK] Da giai nen xong cac goi phu thuoc.

:: ==========================================================================
:: BƯỚC 2: SAO CHÉP MÃ NGUỒN VÀ MAKEFILE (Lớp đè)
:: ==========================================================================
echo.
echo =======================================================
echo [BUOC 2] Dang ap dung lop de ma nguon va Makefile...
echo =======================================================

if not exist "doomgeneric-master\doomgeneric" mkdir "doomgeneric-master\doomgeneric"
if not exist "Custom build Makefile" mkdir "Custom build Makefile"

copy /Y "%CACHE_DIR%\kit-v%KIT_VERSION%\src\doomgeneric_sdl.c" "doomgeneric-master\doomgeneric\doomgeneric_sdl.c" >nul
copy /Y "%CACHE_DIR%\kit-v%KIT_VERSION%\make\Makefile.win" "Custom build Makefile\Makefile.win" >nul
echo [OK] Da ap dung lop de.

:: ==========================================================================
:: BƯỚC 3: CHẠY CÁC SCRIPT POWERSHELL (Tài liệu & Telemetry)
:: ==========================================================================
echo.
echo =======================================================
echo [BUOC 3] Dang cap nhat tai lieu va telemetry...
echo =======================================================

set "KIT_DIR=%CACHE_DIR%\kit-v%KIT_VERSION%"

powershell -NoProfile -ExecutionPolicy Bypass -File "%KIT_DIR%\scripts\install_docs.ps1"

echo.
echo =======================================================
echo [HOAN TAT] Thiet lap moi truong EduDoom-Ass v2.1.0 thanh cong!
echo =======================================================
pause