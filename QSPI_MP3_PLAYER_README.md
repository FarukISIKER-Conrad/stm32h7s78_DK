# QSPI Flash MP3 Player - Kullanım Kılavuzu

## 📋 Genel Bakış

Bu proje, STM32H7S78-DK kartında **QSPI Flash** üzerinden **FatFS** ile MP3 dosyalarını okuyup **minimp3** ile decode ederek çalma özelliğini içerir.

## 🎯 Özellikler

- ✅ QSPI Flash (XSPI2 - 1GB NOR Flash) üzerinden dosya okuma
- ✅ FatFS ile dosya sistemi desteği
- ✅ MP3 decode (minimp3)
- ✅ Real-time streaming (buffer'sız, direkt QSPI'den okuma)
- ✅ Loop playback desteği
- ✅ FreeRTOS entegrasyonu
- ✅ DMA ile kesintisiz audio çalma

## 📁 Dosya Yapısı

```
Appli/
├── Core/
│   ├── Inc/
│   │   └── mp3_file_player.h         # MP3 player API
│   ├── Src/
│   │   ├── mp3_file_player.c         # MP3 player implementasyonu
│   │   └── main.c                    # Test kodu entegrasyonu
└── FATFS/
    ├── App/
    │   └── user_diskio.c             # QSPI Flash disk driver
    └── Target/
        └── ffconf.h                  # FatFS konfigürasyonu
```

## 🔧 Konfigürasyon

### 1. QSPI Flash Bellek Haritası

```c
// user_diskio.c içinde tanımlı
#define QSPI_FATFS_START    0x01000000    // 16MB offset
#define QSPI_FATFS_SIZE     (64*1024*1024) // 64MB alan
```

**Bellek Bölümleri:**
```
0x90000000 - 0x90FFFFFF : Program kodu ve veri (16MB)
0x91000000 - 0x94FFFFFF : FatFS bölümü (64MB)
0x95000000 - 0x9FFFFFFF : Kullanılabilir alan
```

### 2. FatFS Ayarları

`ffconf.h` dosyasında önemli ayarlar:
```c
FF_USE_LFN      = 3      // Long filename (dynamic heap)
FF_CODE_PAGE    = 865    // Nordic (İngilizce dosya isimleri)
FF_FS_RPATH     = 1      // Relative path desteği
FF_FS_REENTRANT = 1      // Thread-safe (FreeRTOS)
FF_MIN_SS       = 512    // Sector size
FF_MAX_SS       = 512
```

## 🚀 Kullanım

### Adım 1: QSPI Flash'i Format Etme

İlk kullanımda QSPI Flash'in FatFS bölümünü format etmeniz gerekir:

```c
// main.c içine ekleyin
FRESULT fres;
MKFS_PARM fmt_opt = {
    .fmt = FM_FAT32,
    .n_fat = 1,
    .align = 0,
    .n_root = 512,
    .au_size = 4096
};

// QSPI Flash'i format et
fres = f_mkfs("0:", &fmt_opt, work_buffer, sizeof(work_buffer));
if (fres == FR_OK) {
    printf("Format basarili!\r\n");
}
```

### Adım 2: MP3 Dosyasını QSPI Flash'e Yükleme

**STM32CubeProgrammer ile:**

1. MP3 dosyanızı hazırlayın: `guitar.mp3`
2. STM32CubeProgrammer'ı açın
3. **External Loader** → `MX66UW1G45G_STM32H7S78-DK` seçin
4. **Erasing & Programming**:
   - File: `guitar.mp3` seçin
   - Start Address: `0x90000000` (memory-mapped base)
   - **Download** tıklayın

**Veya FatFS ile programatik yükleme:**
```c
// PC'den seri port ile dosya transfer edip flash'e yazma
FIL file;
f_open(&file, "0:/guitar.mp3", FA_WRITE | FA_CREATE_ALWAYS);
f_write(&file, mp3_data, mp3_size, &bytes_written);
f_close(&file);
```

### Adım 3: MP3 Çalma

```c
// main.c - audioTaskHandler içinde
void audioTaskHandler(void *argument)
{
    audio_drv_init(&audio_drv);
    audio_drv_start_dma(&audio_drv);
    
    // QSPI Flash mount
    vTaskDelay(pdMS_TO_TICKS(500));
    test_mp3_player_from_qspi();
    
    for(;;) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

## 📊 API Kullanımı

### MP3 Dosyası Açma

```c
mp3_file_player_t player;
int result = mp3_file_player_open(&player, "0:/guitar.mp3");
if (result == MP3_PLAYER_OK) {
    printf("Dosya acildi!\n");
}
```

### Frame Decode Etme

```c
int16_t *pcm_data;
int samples;
int result = mp3_file_player_decode_frame(&player, &pcm_data, &samples);
if (result > 0) {
    // pcm_data: stereo int16 PCM buffer
    // samples: channel başına sample sayısı
    audio_drv_play(&audio_drv, (uint8_t*)pcm_data, samples * 2 * sizeof(int16_t));
}
```

### Loop Playback

```c
mp3_file_player_set_loop(&player, 1);  // Enable loop
```

### Bilgi Alma

```c
uint32_t sample_rate, file_size;
uint8_t channels;
mp3_file_player_get_info(&player, &sample_rate, &channels, &file_size);
printf("Sample Rate: %lu Hz, Channels: %d\n", sample_rate, channels);
```

### Dosyayı Kapatma

```c
mp3_file_player_close(&player);
```

## 🔍 Hata Ayıklama

### Problem: FatFS Mount Hatası

```c
HATA: FatFS mount basarisiz! Kod: 13
```

**Çözüm:**
1. QSPI Flash format edilmemiş olabilir → `f_mkfs()` çalıştırın
2. `QSPI_FATFS_START` adresi yanlış olabilir
3. `user_diskio.c` konfigürasyonunu kontrol edin

### Problem: MP3 Dosyası Açılmıyor

```c
HATA: MP3 dosyasi acilamadi! Kod: -2
```

**Çözüm:**
1. Dosya yolu doğru mu? → `"0:/guitar.mp3"`
2. Dosya QSPI Flash'te var mı? → Directory listesi kontrol edin
3. Dosya ismi doğru mu? (büyük/küçük harf duyarlı)

### Problem: Decode Hatası

```c
HATA: Decode basarisiz! Kod: -3
```

**Çözüm:**
1. MP3 dosyası geçerli mi?
2. MP3 formatı destekleniyor mu? (MPEG-1/2 Layer 3)
3. Bitrate çok yüksek değil mi? (320kbps ve altı önerilir)

## 📈 Performans

### Bellek Kullanımı

```
MP3 Read Buffer:  8 KB
PCM Buffer:       9 KB  (1152 * 4 * 2)
Total RAM:        ~17 KB per player instance
```

### QSPI Flash Hızı

```
Read Speed:  ~50 MB/s (XSPI2 - 8-line mode)
Latency:     <1ms
```

### Audio Specs

```
Supported Formats: MP3 (MPEG-1/2 Layer 3)
Sample Rates:      8kHz - 48kHz
Bitrates:          32kbps - 320kbps
Channels:          Mono/Stereo
Output:            48kHz, 16-bit, Stereo PCM
```

## 🛠️ Özelleştirme

### Buffer Boyutlarını Değiştirme

```c
// mp3_file_player.h
#define MP3_READ_BUFFER_SIZE    (16*1024)  // 8KB → 16KB
```

### FatFS Başlangıç Adresini Değiştirme

```c
// user_diskio.c
#define QSPI_FATFS_START    0x02000000  // 32MB offset
```

## 📝 Notlar

1. **Dosya İsimleri:** İngilizce karakterler kullanın (Türkçe karakter desteği için `FF_CODE_PAGE = 857`)
2. **Wear Leveling:** Bu basit implementasyon wear leveling içermez (production'da ekleyin)
3. **Thread Safety:** FatFS `FF_FS_REENTRANT=1` ile thread-safe
4. **DMA Alignment:** PCM buffer'ları 32-byte aligned

## 🎵 Örnek Projeler

### 1. Basit MP3 Player

```c
play_mp3_file("0:/music/song1.mp3");
```

### 2. Playlist

```c
const char *playlist[] = {
    "0:/music/song1.mp3",
    "0:/music/song2.mp3",
    "0:/music/song3.mp3"
};

for (int i = 0; i < 3; i++) {
    play_mp3_file(playlist[i]);
}
```

### 3. UI ile Kontrol

```c
// TouchGFX button callback
void playButtonPressed() {
    mp3_playback_enabled = 1;
    play_mp3_file(selected_file);
}

void stopButtonPressed() {
    mp3_playback_enabled = 0;
    mp3_file_player_close(&mp3_player);
}
```

## 📚 Referanslar

- [minimp3 GitHub](https://github.com/lieff/minimp3)
- [FatFS Documentation](http://elm-chan.org/fsw/ff/)
- [STM32H7S78-DK User Manual](https://www.st.com/resource/en/user_manual/um3237-stm32h7s78-discovery-kit-stmicroelectronics.pdf)

---

**Tarih:** 23 Aralık 2024  
**Versiyon:** 1.0  
**Yazar:** Faruk Isıker

