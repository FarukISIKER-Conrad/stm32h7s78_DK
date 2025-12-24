@echo off
echo ====================================
echo Dog MP3 Oynatici - Derleme ve Calistirma
echo ====================================
echo.

REM Eski exe dosyasini sil
if exist play_dog_sound.exe del play_dog_sound.exe

echo Derleniyor...
gcc -o play_dog_sound.exe play_dog_sound.c dog_mp3_data.c -I../Appli/Core/Inc -lwinmm -O2

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo HATA: Derleme basarisiz!
    pause
    exit /b 1
)

echo.
echo Derleme basarili!
echo.
echo Oynatiliyor...
echo.

play_dog_sound.exe

echo.
pause

