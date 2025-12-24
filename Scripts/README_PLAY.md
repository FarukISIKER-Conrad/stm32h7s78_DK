# Dog MP3 Oynatıcı

Bu program `dog_mp3_data.c` dosyasındaki MP3 verisini Windows'ta bilgisayardan oynatır.

## Gereksinimler

- GCC (MinGW veya MSYS2)
- Windows 11 (veya herhangi bir Windows sürümü)

## Kullanım

### Otomatik Derleme ve Çalıştırma

```batch
compile_and_play.bat
```

### Manuel Derleme

```batch
gcc -o play_dog_sound.exe play_dog_sound.c dog_mp3_data.c -I../Appli/Core/Inc -lwinmm -O2
```

### Çalıştırma

```batch
play_dog_sound.exe
```

## Nasıl Çalışır?

1. **MP3 Decode**: minimp3 kütüphanesi kullanılarak `dog_mp3_file_data[]` dizisindeki MP3 verisi PCM formatına dönüştürülür.

2. **Ses Oynatma**: Windows Multimedia API (`winmm.dll`) kullanılarak PCM verisi ses kartından oynatılır.

3. **API Fonksiyonları**:
   - `waveOutOpen()`: Ses cihazını açar
   - `waveOutPrepareHeader()`: Ses verisini hazırlar
   - `waveOutWrite()`: Sesi oynatır
   - `waveOutClose()`: Ses cihazını kapatır

## Dosyalar

- `play_dog_sound.c`: Ana program dosyası
- `dog_mp3_data.c`: MP3 verisi (dog.mp3'ten dönüştürülmüş)
- `compile_and_play.bat`: Otomatik derleme ve çalıştırma scripti
- `../Appli/Core/Inc/minimp3.h`: MP3 decoder kütüphanesi

## Çıktı Örneği

```
=== Dog MP3 Oynatici ===
MP3 Dosya Boyutu: 192768 bytes

MP3 decode ediliyor...
Sample Rate: 44100 Hz, Channels: 2
Toplam 419328 sample decode edildi (4.75 saniye)
Ses oynatiliyor...
Ses oynatma tamamlandi!
```

## Sorun Giderme

### "gcc: command not found"
MinGW veya MSYS2 kurulu olmalı ve PATH'e eklenmiş olmalıdır.

### "Hata: Ses cihazi acilamadi!"
- Ses kartınızın düzgün çalıştığından emin olun
- Başka bir program ses kartını kullanıyor olabilir
- Windows ses ayarlarını kontrol edin

### Derleme Hatası
- GCC'nin düzgün kurulu olduğundan emin olun
- minimp3.h dosyasının doğru yolda olduğunu kontrol edin

