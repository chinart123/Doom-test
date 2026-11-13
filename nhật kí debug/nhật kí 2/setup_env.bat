@echo off
setlocal EnableDelayedExpansion
chcp 65001 >nul
title EduDoom-Ass v2.1.0 Setup

:: ==========================================================================
:: CẤU HÌNH KIT
:: ==========================================================================
set KIT_VERSION=2.1.0
set KIT_URL=file:///E:/Workspace/Doom/v2.1.0-Level-1A/test/_tool/build_kit.ps1/kit-v2.1.0.zip   
set KIT_SHA256=6489ca3e2fda0e83ad7536bf666592f4dd13e6a2ff62ae4ff607cb54167cb08a

:: Tu dong lay duong dan hien tai cua file .bat
::set "BAT_DIR=%~dp0"
::set "BAT_DIR=%BAT_DIR:\=/%"
::set "BAT_DIR=%BAT_DIR:~0,-1%"
::set "KIT_URL=file:///%BAT_DIR%/kit-v%KIT_VERSION%.zip"
:: DÁN MÃ HASH BẠN VỪA COPY VÀO ĐÂY:
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

if not exist "%CACHE_DIR%\kit-v%KIT_VERSION%.zip" (
    echo Dang tai kit-v%KIT_VERSION%.zip...
    powershell -Command "Invoke-WebRequest -Uri '%KIT_URL%' -OutFile '%CACHE_DIR%\kit-v%KIT_VERSION%.zip'"
)

echo Dang kiem tra ma bam SHA256...
for /f "tokens=*" %%H in ('powershell -Command "(Get-FileHash -Path '%CACHE_DIR%\kit-v%KIT_VERSION%.zip' -Algorithm SHA256).Hash.ToLower()"') do set "ACTUAL_HASH=%%H"
if not "%ACTUAL_HASH%"=="%KIT_SHA256%" (
    echo [LOI] SHA256 KHONG KHOP! File bi hong hoac bi chinh sua.
    pause
    exit /b 1
)
echo [OK] SHA256 khop.

echo Dang giai nen kit...
if exist "%CACHE_DIR%\kit-v%KIT_VERSION%" rmdir /S /Q "%CACHE_DIR%\kit-v%KIT_VERSION%"
mkdir "%CACHE_DIR%\kit-v%KIT_VERSION%"
tar -xf "%CACHE_DIR%\kit-v%KIT_VERSION%.zip" -C "%CACHE_DIR%\kit-v%KIT_VERSION%"
echo [OK] Da giai nen kit.

:: ==========================================================================
:: BƯỚC 2: SAO CHÉP MÃ NGUỒN VÀ MAKEFILE (Lớp đè)
:: ==========================================================================
echo.
echo =======================================================
echo [BUOC 2] Dang ap dung lop de ma nguon va Makefile...
echo =======================================================

if not exist "doomgeneric-master\doomgeneric" mkdir "doomgeneric-master\doomgeneric"
if not exist "Custom build Makefile" mkdir "Custom build Makefile"
if not exist "wads" mkdir "wads"
if not exist "third_party" mkdir "third_party"

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
set "PROJECT_DIR=%PROJECT_DIR%"

powershell -NoProfile -ExecutionPolicy Bypass -File "%KIT_DIR%\scripts\install_docs.ps1"

echo.
echo =======================================================
echo [HOAN TAT] Thiet lap moi truong EduDoom-Ass v2.1.0 thanh cong!
echo =======================================================
pause