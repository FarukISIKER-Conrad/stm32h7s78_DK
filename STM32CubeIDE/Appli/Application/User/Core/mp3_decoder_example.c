/*
 * mp3_decoder_example.c
 *
 * MP3 Decoder kullanım örnekleri
 *
 * Created on: Dec 22, 2025
 * Author: Faruk Isıker
 */

#include "mp3_decoder.h"
#include <stdio.h>

/* Örnek MP3 dosya verisi (normalde Flash veya SD Kart'tan gelir) */
extern const uint8_t mp3_file_data[];
extern const size_t mp3_file_size;

/* Çıkış buffer'ı - 48kHz, stereo, 16-bit */
#define OUTPUT_BUFFER_SIZE (48000 * 2 * 10)  /* 10 saniye için buffer */
static int16_t pcm_output_buffer[OUTPUT_BUFFER_SIZE];

/**
 * @brief Örnek 1: Basit MP3 decode işlemi
 * 
 * Bu örnek bir MP3 dosyasını tamamen decode eder ve 
 * PCM output buffer'ına yazar.
 */
void example_simple_decode(void)
{
    mp3_decoder_handle_t decoder;
    size_t samples_decoded = 0;
    int result;
    
    /* 1. Decoder'ı başlat */
    result = mp3_decoder_init(&decoder);
    if (result != MP3_DEC_OK) {
        printf("Decoder init hatası: %d\n", result);
        return;
    }
    
    /* 2. MP3 dosyasını yükle */
    result = mp3_decoder_load(&decoder, mp3_file_data, mp3_file_size);
    if (result != MP3_DEC_OK) {
        printf("MP3 yükleme hatası: %d\n", result);
        return;
    }
    
    /* 3. MP3'ü decode et ve output buffer'a yaz */
    result = mp3_decoder_process(&decoder, pcm_output_buffer, OUTPUT_BUFFER_SIZE, &samples_decoded);
    if (result != MP3_DEC_OK) {
        printf("Decode hatası: %d\n", result);
        return;
    }
    
    printf("Decode tamamlandı! %zu sample decoded edildi.\n", samples_decoded);
    
    /* 4. Bilgileri al */
    uint32_t sample_rate, bitrate;
    uint8_t channels;
    mp3_decoder_get_info(&decoder, &sample_rate, &channels, &bitrate);
    printf("Sample Rate: %lu Hz\n", sample_rate);
    printf("Channels: %u\n", channels);
    printf("Bitrate: %lu kbps\n", bitrate);
    
    /* 5. Şimdi pcm_output_buffer audio driver'a gönderilebilir */
    /* audio_drv_play_buffer(pcm_output_buffer, samples_decoded); */
}

/**
 * @brief Örnek 2: Frame-by-frame decode (streaming için)
 * 
 * Bu örnek MP3'ü frame by frame decode eder.
 * DMA callback'lerle kullanım için uygundur.
 */
void example_streaming_decode(void)
{
    mp3_decoder_handle_t decoder;
    size_t frame_samples = 0;
    int result;
    
    /* Decoder'ı başlat */
    mp3_decoder_init(&decoder);
    mp3_decoder_load(&decoder, mp3_file_data, mp3_file_size);
    mp3_decoder_set_output_buffer(&decoder, pcm_output_buffer, OUTPUT_BUFFER_SIZE);
    
    /* Frame by frame decode */
    while (1) {
        result = mp3_decoder_decode_frame(&decoder, &frame_samples);
        
        if (result == MP3_DEC_END_OF_FILE) {
            printf("Dosya sonu\n");
            break;
        }
        
        if (result == MP3_DEC_OK) {
            printf("Frame decoded: %zu samples\n", frame_samples);
            /* Bu frame'i DMA'ya gönder */
        }
        
        if (result == MP3_DEC_NEED_MORE_DATA) {
            /* Geçersiz frame, bir sonrakine geç */
            continue;
        }
    }
}

/**
 * @brief Örnek 3: Audio driver ile entegrasyon
 * 
 * Bu örnek audio_drv ile entegrasyonu gösterir.
 */
void example_audio_driver_integration(audio_drv_t *audio_drv)
{
    mp3_decoder_handle_t decoder;
    size_t samples_decoded = 0;
    
    /* Decoder'ı başlat */
    mp3_decoder_init(&decoder);
    mp3_decoder_load(&decoder, mp3_file_data, mp3_file_size);
    
    /* MP3'ü decode et */
    mp3_decoder_process(&decoder, pcm_output_buffer, OUTPUT_BUFFER_SIZE, &samples_decoded);
    
    /* Audio driver'ı güncelle */
    /* audio_drv->sine.p_tx_data yerine decoded PCM data kullan */
    /* 
    audio_drv->sine.p_tx_data = pcm_output_buffer;
    audio_drv->sine.tx_data_size = samples_decoded;
    audio_drv_start_dma(audio_drv);
    */
}

/**
 * @brief Örnek 4: DMA half/complete callback'ler ile circular buffer
 * 
 * Bu örnek circular DMA buffer ile streaming decode gösterir.
 */
#define DMA_BUFFER_SIZE (4096)  /* DMA buffer boyutu */
static int16_t dma_buffer[DMA_BUFFER_SIZE];
static mp3_decoder_handle_t g_decoder;

void setup_streaming_playback(void)
{
    /* Decoder'ı başlat */
    mp3_decoder_init(&g_decoder);
    mp3_decoder_load(&g_decoder, mp3_file_data, mp3_file_size);
    mp3_decoder_set_output_buffer(&g_decoder, dma_buffer, DMA_BUFFER_SIZE);
    
    /* İlk buffer'ı doldur */
    size_t samples = 0;
    while (samples < DMA_BUFFER_SIZE / 2) {
        size_t frame_samples = 0;
        mp3_decoder_decode_frame(&g_decoder, &frame_samples);
        samples += frame_samples;
    }
    
    /* DMA'yı başlat */
    /* HAL_SAI_Transmit_DMA(hsai, (uint8_t*)dma_buffer, DMA_BUFFER_SIZE); */
}

/* DMA Half Complete Callback */
void dma_half_complete_callback(void)
{
    /* İlk yarıyı doldur */
    size_t samples = 0;
    size_t half_size = DMA_BUFFER_SIZE / 2;
    
    while (samples < half_size) {
        size_t frame_samples = 0;
        int result = mp3_decoder_decode_frame(&g_decoder, &frame_samples);
        
        if (result == MP3_DEC_END_OF_FILE) {
            /* Dosya bitti, başa dön veya durdur */
            mp3_decoder_reset(&g_decoder);
            break;
        }
        
        samples += frame_samples;
    }
}

/* DMA Complete Callback */
void dma_complete_callback(void)
{
    /* İkinci yarıyı doldur */
    size_t samples = 0;
    size_t half_size = DMA_BUFFER_SIZE / 2;
    
    /* output_buffer_position'ı manuel ayarla veya ikinci buffer kullan */
    g_decoder.output_buffer = &dma_buffer[half_size];
    g_decoder.output_buffer_position = 0;
    
    while (samples < half_size) {
        size_t frame_samples = 0;
        int result = mp3_decoder_decode_frame(&g_decoder, &frame_samples);
        
        if (result == MP3_DEC_END_OF_FILE) {
            mp3_decoder_reset(&g_decoder);
            break;
        }
        
        samples += frame_samples;
    }
    
    /* Buffer'ı geri ayarla */
    g_decoder.output_buffer = dma_buffer;
}

/**
 * @brief Örnek 5: MP3 dosya bilgilerini okuma
 */
void example_get_mp3_info(void)
{
    mp3_decoder_handle_t decoder;
    uint32_t sample_rate, bitrate;
    uint8_t channels;
    
    mp3_decoder_init(&decoder);
    mp3_decoder_load(&decoder, mp3_file_data, mp3_file_size);
    
    /* İlk frame'i decode et (bilgi almak için) */
    int16_t temp_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME];
    mp3_decoder_set_output_buffer(&decoder, temp_buffer, MINIMP3_MAX_SAMPLES_PER_FRAME);
    mp3_decoder_decode_frame(&decoder, NULL);
    
    /* Bilgileri al */
    mp3_decoder_get_info(&decoder, &sample_rate, &channels, &bitrate);
    
    printf("MP3 Dosya Bilgileri:\n");
    printf("  Sample Rate: %lu Hz\n", sample_rate);
    printf("  Channels: %u (%s)\n", channels, channels == 2 ? "Stereo" : "Mono");
    printf("  Bitrate: %lu kbps\n", bitrate);
    
    /* Toplam süreyi hesapla (yaklaşık) */
    float duration_sec = (float)mp3_file_size / ((float)bitrate * 125.0f);
    // printf("  Süre: %.2f saniye\n", duration_sec);
}

