@echo off
REM MP3 Decoder Setup Script
REM Bu script minimp3 kutuphanesini otomatik olarak indirir ve kurar

echo ========================================
echo MP3 Decoder Kurulum
echo ========================================
echo.

REM minimp3.h dosyasinin yolunu ayarla
set MINIMP3_H=Appli\Core\Inc\minimp3.h
set MINIMP3_C=Appli\Core\Src\minimp3.c
set MINIMP3_URL=https://raw.githubusercontent.com/lieff/minimp3/master/minimp3.h

echo 1. minimp3.h indiriliyor...
echo URL: %MINIMP3_URL%
echo Hedef: %MINIMP3_H%
echo.

REM curl veya PowerShell ile indir
where curl >nul 2>&1
if %errorlevel% equ 0 (
    echo curl kullanilarak indiriliyor...
    curl -L -o "%MINIMP3_H%" "%MINIMP3_URL%"
) else (
    echo PowerShell kullanilarak indiriliyor...
    powershell -Command "Invoke-WebRequest -Uri '%MINIMP3_URL%' -OutFile '%MINIMP3_H%'"
)

if not exist "%MINIMP3_H%" (
    echo.
    echo HATA: minimp3.h indirilemedi!
    echo Manuel olarak su adresten indirin:
    echo %MINIMP3_URL%
    echo.
    pause
    exit /b 1
)

echo.
echo 2. minimp3.c guncelleniyor...

REM minimp3.c dosyasini guncelle
(
echo /*
echo  * minimp3.c - Minimalistic MP3 decoder implementation
echo  * 
echo  * Full minimp3 library implementation
echo  * https://github.com/lieff/minimp3
echo  */
echo.
echo #define MINIMP3_IMPLEMENTATION
echo #define MINIMP3_NO_SIMD  /* Disable SIMD for ARM compatibility */
echo.
echo #include "minimp3.h"
) > "%MINIMP3_C%"

echo.
echo ========================================
echo Kurulum Tamamlandi!
echo ========================================
echo.
echo MP3 decoder kutuphanesi kullanima hazir.
echo.
echo Sonraki adimlar:
echo 1. Projenizi STM32CubeIDE'de acin
echo 2. Projeyi derleyin (Build)
echo 3. mp3_decoder_example.c dosyasindaki ornekleri inceleyin
echo 4. MP3_DECODER_README.md dosyasini okuyun
echo.
echo Kullanim ornegi:
echo   mp3_decoder_handle_t decoder;
echo   mp3_decoder_init(^&decoder);
echo   mp3_decoder_load(^&decoder, mp3_data, mp3_size);
echo   mp3_decoder_process(^&decoder, output_buffer, size, ^&samples);
echo.
pause

