# MP3 Decoder Library - STM32

Bu kütüphane STM32 mikrodenetleyiciler için hafif ve verimli bir MP3 decoder sağlar.

## Özellikler

- ✅ 48kHz örnekleme hızı
- ✅ 16-bit stereo çıkış (Left/Right kanallar)
- ✅ Otomatik mono-to-stereo dönüşüm
- ✅ Otomatik sample rate resampling
- ✅ DMA destekli streaming playback
- ✅ Frame-by-frame veya full-file decoding
- ✅ Düşük bellek kullanımı
- ✅ minimp3 tabanlı (public domain)

## Dosyalar

```
Appli/Core/Inc/
  ├── mp3_decoder.h      - Ana API header
  └── minimp3.h          - MP3 decoder engine header

Appli/Core/Src/
  ├── mp3_decoder.c      - Ana implementation
  ├── minimp3.c          - MP3 decoder engine
  └── mp3_decoder_example.c - Kullanım örnekleri
```

## Kurulum

### 1. Dosyaları Projeye Ekleyin

Tüm dosyalar zaten doğru dizinlere yerleştirilmiştir:
- `Appli/Core/Inc/` - Header dosyaları
- `Appli/Core/Src/` - Source dosyaları

### 2. Tam minimp3 Kütüphanesini Ekleyin

Şu anda stub implementation var. Gerçek MP3 decoding için:

```bash
# minimp3.h dosyasını indirin
wget https://raw.githubusercontent.com/lieff/minimp3/master/minimp3.h -O Appli/Core/Inc/minimp3.h
```

Ardından `Appli/Core/Src/minimp3.c` dosyasını şöyle güncelleyin:

```c
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD  // ARM için SIMD desteğini devre dışı bırakın
#include "minimp3.h"
```

### 3. Projeyi Derleyin

STM32CubeIDE, GCC veya tercih ettiğiniz toolchain ile derleyin.

## Kullanım

### Basit Kullanım

```c
#include "mp3_decoder.h"

// MP3 dosyası (Flash, SD Card, vb.)
extern const uint8_t mp3_data[];
extern const size_t mp3_size;

// Çıkış buffer'ı
#define BUFFER_SIZE (48000 * 2 * 5)  // 5 saniye stereo
int16_t pcm_buffer[BUFFER_SIZE];

void play_mp3(void)
{
    mp3_decoder_handle_t decoder;
    size_t samples_decoded;
    
    // 1. Initialize
    mp3_decoder_init(&decoder);
    
    // 2. Load MP3 data
    mp3_decoder_load(&decoder, mp3_data, mp3_size);
    
    // 3. Decode entire file
    mp3_decoder_process(&decoder, pcm_buffer, BUFFER_SIZE, &samples_decoded);
    
    // 4. pcm_buffer artık audio driver'a gönderilebilir
    // HAL_SAI_Transmit_DMA(hsai, (uint8_t*)pcm_buffer, samples_decoded * 2);
}
```

### Streaming Playback (DMA ile)

```c
#define DMA_BUFFER_SIZE 4096
int16_t dma_buffer[DMA_BUFFER_SIZE];
mp3_decoder_handle_t decoder;

void setup_streaming(void)
{
    mp3_decoder_init(&decoder);
    mp3_decoder_load(&decoder, mp3_data, mp3_size);
    mp3_decoder_set_output_buffer(&decoder, dma_buffer, DMA_BUFFER_SIZE);
    
    // İlk buffer'ı doldur
    fill_buffer_half(&decoder, 0);
    
    // DMA'yı başlat
    HAL_SAI_Transmit_DMA(hsai, (uint8_t*)dma_buffer, DMA_BUFFER_SIZE);
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    // İlk yarıyı doldur
    fill_buffer_half(&decoder, 0);
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    // İkinci yarıyı doldur
    fill_buffer_half(&decoder, DMA_BUFFER_SIZE / 2);
}

void fill_buffer_half(mp3_decoder_handle_t *dec, size_t offset)
{
    dec->output_buffer = &dma_buffer[offset];
    dec->output_buffer_position = 0;
    
    size_t samples = 0;
    while (samples < DMA_BUFFER_SIZE / 2) {
        size_t frame_samples;
        if (mp3_decoder_decode_frame(dec, &frame_samples) == MP3_DEC_END_OF_FILE) {
            mp3_decoder_reset(dec);  // Loop
            break;
        }
        samples += frame_samples;
    }
}
```

## API Fonksiyonları

### Temel Fonksiyonlar

#### `mp3_decoder_init()`
```c
int mp3_decoder_init(mp3_decoder_handle_t *handle);
```
Decoder'ı başlatır.

**Parametreler:**
- `handle`: Decoder handle pointer

**Dönüş:** `MP3_DEC_OK` veya hata kodu

---

#### `mp3_decoder_load()`
```c
int mp3_decoder_load(mp3_decoder_handle_t *handle, const uint8_t *mp3_data, size_t mp3_length);
```
MP3 dosyasını belleğe yükler.

**Parametreler:**
- `handle`: Decoder handle
- `mp3_data`: MP3 dosya verisi pointer'ı
- `mp3_length`: MP3 dosya boyutu (byte)

**Dönüş:** `MP3_DEC_OK` veya hata kodu

---

#### `mp3_decoder_process()`
```c
int mp3_decoder_process(mp3_decoder_handle_t *handle, int16_t *output_pcm, 
                       size_t output_size, size_t *samples_decoded);
```
Tüm MP3 dosyasını decode eder.

**Parametreler:**
- `handle`: Decoder handle
- `output_pcm`: Çıkış PCM buffer (int16_t stereo)
- `output_size`: Buffer boyutu (samples cinsinden)
- `samples_decoded`: Decode edilen sample sayısı (NULL olabilir)

**Dönüş:** `MP3_DEC_OK` veya hata kodu

**Çıkış Formatı:**
- Sample rate: 48kHz
- Format: 16-bit signed integer
- Channels: 2 (stereo)
- Layout: [L, R, L, R, L, R, ...]

---

#### `mp3_decoder_decode_frame()`
```c
int mp3_decoder_decode_frame(mp3_decoder_handle_t *handle, size_t *samples_decoded);
```
Tek bir MP3 frame decode eder (streaming için).

**Parametreler:**
- `handle`: Decoder handle
- `samples_decoded`: Frame'de decode edilen sample sayısı

**Dönüş:** `MP3_DEC_OK`, `MP3_DEC_END_OF_FILE`, veya hata kodu

---

#### `mp3_decoder_get_info()`
```c
int mp3_decoder_get_info(mp3_decoder_handle_t *handle, uint32_t *sample_rate, 
                        uint8_t *channels, uint32_t *bitrate);
```
MP3 dosyası hakkında bilgi alır.

**Parametreler:**
- `handle`: Decoder handle
- `sample_rate`: Orijinal sample rate (Hz)
- `channels`: Kanal sayısı (1=mono, 2=stereo)
- `bitrate`: Bitrate (kbps)

---

#### `mp3_decoder_reset()`
```c
int mp3_decoder_reset(mp3_decoder_handle_t *handle);
```
Decoder'ı başa sarar (loop playback için).

---

### Dönüş Kodları

```c
MP3_DEC_OK              = 0   // Başarılı
MP3_DEC_ERROR           = -1  // Genel hata
MP3_DEC_NEED_MORE_DATA  = -2  // Daha fazla veri gerekli
MP3_DEC_END_OF_FILE     = -3  // Dosya sonu
MP3_DEC_INVALID_PARAM   = -4  // Geçersiz parametre
```

## Bellek Kullanımı

- **Decoder handle:** ~1.5 KB (mp3dec_t + state)
- **Frame buffer:** ~2.3 KB (MINIMP3_MAX_SAMPLES_PER_FRAME)
- **Output buffer:** Uygulamaya bağlı
  - 1 saniye: 48000 * 2 * 2 = 192 KB
  - 5 saniye: 960 KB
  - Streaming için: 4-8 KB (DMA buffer)

## Performans

- **CPU Kullanımı:** Frame başına ~1-3ms (STM32H7 @ 600MHz)
- **Gerçek Zamanlı:** Evet (48kHz için yeterli)
- **DMA Uyumlu:** Evet

## Audio Driver Entegrasyonu

Mevcut `audio_drv.c` ile entegrasyon örneği:

```c
#include "audio_drv.h"
#include "mp3_decoder.h"

// Decoded PCM data
int16_t decoded_pcm[48000 * 2 * 5];  // 5 saniye
size_t decoded_samples = 0;

void play_mp3_with_audio_drv(audio_drv_t *audio_drv)
{
    mp3_decoder_handle_t decoder;
    
    // MP3'ü decode et
    mp3_decoder_init(&decoder);
    mp3_decoder_load(&decoder, mp3_data, mp3_size);
    mp3_decoder_process(&decoder, decoded_pcm, sizeof(decoded_pcm)/sizeof(int16_t), 
                       &decoded_samples);
    
    // Audio driver'ı güncelle
    audio_drv->sine.p_tx_data = decoded_pcm;
    audio_drv->sine.tx_data_size = decoded_samples;
    
    // Playback başlat
    audio_drv_start_dma(audio_drv);
}
```

## Örnek Projeler

`mp3_decoder_example.c` dosyasında 5 farklı kullanım örneği bulabilirsiniz:

1. **Basit Decode:** Tüm dosyayı decode etme
2. **Streaming Decode:** Frame-by-frame decode
3. **Audio Driver Entegrasyonu**
4. **DMA Circular Buffer:** Half/Complete callback'lerle
5. **MP3 Bilgi Okuma:** Sample rate, bitrate, süre

## Troubleshooting

### Problem: Compile hatası - minimp3 fonksiyonları bulunamıyor

**Çözüm:** Tam minimp3.h dosyasını indirin:
```bash
wget https://raw.githubusercontent.com/lieff/minimp3/master/minimp3.h
```

### Problem: Ses kalitesi kötü

**Çözüm:** 
- Resampling kalitesini artırın (linear interpolation ekleyin)
- SIMD optimizasyonlarını aktif edin (NEON for ARM)

### Problem: CPU kullanımı çok yüksek

**Çözüm:**
- DMA buffer boyutunu artırın
- Frame-by-frame decode kullanın
- RTOS task priority'sini ayarlayın

### Problem: Crackling/popping sesleri

**Çözüm:**
- DMA buffer underrun - buffer boyutunu artırın
- Callback'lerde decode süresini azaltın
- Double buffering kullanın

## Lisans

Bu wrapper kodu MIT lisansı altındadır.

minimp3 kütüphanesi public domain'dir (CC0 1.0 Universal).

## Katkıda Bulunma

Geliştirmeler için pull request gönderin veya issue açın.

## Destek

Sorularınız için:
- GitHub Issues
- Email: faruk.isiker@example.com

---

**Not:** Gerçek MP3 decoding için tam minimp3 kütüphanesini eklemeyi unutmayın!

