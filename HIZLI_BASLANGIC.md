# 🎵 QSPI Flash MP3 Player - Hızlı Başlangıç

## ✅ Yapılanlar

FatFS aktif edildi ve QSPI Flash üzerinden MP3 okuma sistemi hazır!

### 📦 Eklenen Dosyalar

1. **`Appli/Core/Inc/mp3_file_player.h`** - MP3 player API
2. **`Appli/Core/Src/mp3_file_player.c`** - MP3 player implementasyonu
3. **`Appli/FATFS/App/user_diskio.c`** - QSPI Flash disk driver (düzenlendi)
4. **`Appli/Core/Src/main.c`** - Test kodu eklendi

### 🔧 Yapılan Değişiklikler

- ✅ FatFS QSPI Flash için konfigure edildi
- ✅ user_diskio.c: EXTMEM API ile QSPI okuma/yazma
- ✅ MP3 file player: FatFS + minimp3 entegrasyonu
- ✅ main.c: Otomatik test kodu eklendi
- ✅ Loop playback desteği

## 🚀 Kullanım Adımları

### 1️⃣ QSPI Flash'i Format Edin (İlk Kullanım)

```c
// main.c içinde (tek seferlik)
FRESULT fres;
BYTE work[FF_MAX_SS];
fres = f_mkfs("0:", 0, work, sizeof(work));
```

### 2️⃣ MP3 Dosyasını QSPI Flash'e Yükleyin

**Yöntem A: STM32CubeProgrammer**
- File: `guitar.mp3`
- Address: `0x91000000` (FatFS start)
- External Loader: `MX66UW1G45G_STM32H7S78-DK`

**Yöntem B: Programatik**
```c
// Seri port ile MP3 verisini alıp yazın
FIL fil;
f_open(&fil, "0:/guitar.mp3", FA_WRITE | FA_CREATE_ALWAYS);
f_write(&fil, mp3_data, size, &bw);
f_close(&fil);
```

### 3️⃣ Projeyi Derleyin ve Çalıştırın

Kod otomatik olarak şunları yapar:
1. QSPI Flash'i FatFS olarak mount eder
2. Root directory'yi listeler
3. `0:/guitar.mp3` dosyasını arar ve çalar
4. Loop mode ile sürekli çalar

## 📊 Bellek Haritası

```
QSPI Flash (XSPI2):
0x90000000 - 0x90FFFFFF : Program/Kod (16MB)
0x91000000 - 0x94FFFFFF : FatFS Alan (64MB)  ← MP3 dosyaları buraya
0x95000000 - 0x9FFFFFFF : Boş alan
```

## 🎯 Test Çıktısı (Seri Port)

Başarılı çalışma:
```
=== QSPI FatFS Mount Test ===
FatFS mount basarili!

Root dizini:
  guitar.mp3 (3516896 bytes)

MP3 dosyasi araniyor...

=== MP3 Player Test ===
Dosya: 0:/guitar.mp3
MP3 dosyasi acildi!
Sample Rate: 48000 Hz
Channels: 2
File Size: 3516896 bytes

Calma basladi...
```

## 🔧 Sorun Giderme

### ❌ "FatFS mount basarisiz!"
**Çözüm:** QSPI Flash format edin (`f_mkfs`)

### ❌ "MP3 dosyasi acilamadi!"
**Çözüm:** 
- Dosya yolu doğru mu? → `"0:/guitar.mp3"`
- Dosya QSPI Flash'te var mı?
- STM32CubeProgrammer ile dosyayı doğru adrese yükledin mi?

### ❌ "Decode basarisiz!"
**Çözüm:** MP3 formatını kontrol edin (MPEG-1/2 Layer 3, max 320kbps)

## 📝 Önemli Notlar

1. **İlk Kullanım:** QSPI Flash'in FatFS bölümünü format etmelisiniz
2. **Dosya İsimleri:** İngilizce karakterler kullanın
3. **Adres:** `QSPI_FATFS_START = 0x01000000` (physical), memory-mapped: `0x91000000`
4. **Test Kodu:** `audioTaskHandler()` içinde otomatik çalışır

## 🎵 API Örneği

```c
// MP3 Aç
mp3_file_player_t player;
mp3_file_player_open(&player, "0:/music/song.mp3");

// Çal (loop)
mp3_file_player_set_loop(&player, 1);
while(1) {
    int16_t *pcm;
    int samples;
    mp3_file_player_decode_frame(&player, &pcm, &samples);
    audio_play(pcm, samples);
}

// Kapat
mp3_file_player_close(&player);
```

## 📚 Detaylı Dokümantasyon

**`QSPI_MP3_PLAYER_README.md`** dosyasına bakın.

---

**Hazır! Projeyi derleyin ve test edin.** 🎉

