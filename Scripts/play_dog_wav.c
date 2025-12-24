// Windows'ta dog_mp3_data.c'den WAV dosyası oluşturup oynatma
// GCC ile derleme: gcc -o play_dog_wav play_dog_wav.c dog_mp3_data.c -lwinmm
// Çalıştırma: play_dog_wav.exe

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>

// minimp3 kütüphanesini include et
#define MINIMP3_IMPLEMENTATION
#include "../Appli/Core/Inc/minimp3.h"

// dog_mp3_data.c dosyasından gelen extern değişkenler
extern const uint8_t dog_mp3_file_data[];
extern const uint32_t dog_mp3_file_size;

// WAV header yapısı
#pragma pack(push, 1)
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // Dosya boyutu - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // Format chunk boyutu (16)
    uint16_t audio_format;  // 1 = PCM
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];           // "data"
    uint32_t data_size;     // PCM veri boyutu
} WAV_Header;
#pragma pack(pop)

// MP3'ü decode et ve WAV dosyası oluştur
int create_wav_file(const char *filename) {
    mp3dec_t mp3d;
    mp3dec_frame_info_t info;
    
    printf("MP3 decode ediliyor...\n");
    
    // İlk frame'i decode et - sample rate ve channel bilgisi için
    int16_t first_frame_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    mp3dec_init(&mp3d);
    int samples = mp3dec_decode_frame(&mp3d, dog_mp3_file_data, dog_mp3_file_size, 
                                     first_frame_pcm, &info);
    
    if (samples == 0) {
        printf("Hata: MP3 decode edilemedi!\n");
        return 0;
    }
    
    int sample_rate = info.hz;
    int channels = info.channels;
    
    printf("  Sample Rate: %d Hz\n", sample_rate);
    printf("  Channels: %d\n", channels);
    
    // Tüm MP3'ü decode et
    mp3dec_init(&mp3d); // Yeniden başlat
    
    // Büyük PCM buffer ayır
    size_t max_samples = dog_mp3_file_size * 10;
    int16_t *pcm_buffer = (int16_t *)malloc(max_samples * sizeof(int16_t));
    if (!pcm_buffer) {
        printf("Hata: Bellek ayrilamadi!\n");
        return 0;
    }
    
    size_t total_samples = 0;
    size_t offset = 0;
    int frame_count = 0;
    
    while (offset < dog_mp3_file_size) {
        int16_t frame_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        samples = mp3dec_decode_frame(&mp3d, dog_mp3_file_data + offset, 
                                     dog_mp3_file_size - offset, frame_pcm, &info);
        
        if (samples == 0 || info.frame_bytes == 0) {
            break;
        }
        
        if (total_samples + samples > max_samples) {
            printf("Hata: PCM buffer dolu!\n");
            break;
        }
        
        memcpy(pcm_buffer + total_samples, frame_pcm, samples * sizeof(int16_t));
        total_samples += samples;
        offset += info.frame_bytes;
        frame_count++;
    }
    
    printf("  Decode edildi: %zu samples (%d frames, %.2f saniye)\n", 
           total_samples, frame_count,
           (float)total_samples / sample_rate / channels);
    
    // WAV dosyası oluştur
    FILE *wav_file = fopen(filename, "wb");
    if (!wav_file) {
        printf("Hata: WAV dosyasi olusturulamadi!\n");
        free(pcm_buffer);
        return 0;
    }
    
    // WAV header'ı doldur
    WAV_Header header;
    size_t data_size = total_samples * sizeof(int16_t);
    
    memcpy(header.riff, "RIFF", 4);
    header.file_size = 36 + data_size;
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.fmt_size = 16;
    header.audio_format = 1; // PCM
    header.num_channels = channels;
    header.sample_rate = sample_rate;
    header.byte_rate = sample_rate * channels * 2; // 2 = 16 bits / 8
    header.block_align = channels * 2;
    header.bits_per_sample = 16;
    memcpy(header.data, "data", 4);
    header.data_size = data_size;
    
    // Header ve PCM verisini yaz
    fwrite(&header, sizeof(WAV_Header), 1, wav_file);
    fwrite(pcm_buffer, sizeof(int16_t), total_samples, wav_file);
    
    fclose(wav_file);
    free(pcm_buffer);
    
    printf("WAV dosyasi olusturuldu: %s (%zu bytes)\n", filename, sizeof(WAV_Header) + data_size);
    
    return 1;
}

int main() {
    printf("=== Dog MP3 -> WAV Oynatici ===\n");
    printf("MP3 Boyutu: %u bytes\n\n", dog_mp3_file_size);
    
    const char *wav_filename = "dog_temp.wav";
    
    // WAV dosyası oluştur
    if (!create_wav_file(wav_filename)) {
        printf("Hata: WAV dosyasi olusturulamadi!\n");
        return 1;
    }
    
    printf("\nSes oynatiliyor...\n");
    
    // PlaySound ile oynat - çok daha basit ve güvenilir!
    if (!PlaySound(wav_filename, NULL, SND_FILENAME | SND_SYNC)) {
        printf("Hata: Ses oynatılamadi!\n");
        return 1;
    }
    
    printf("Ses oynatma tamamlandi!\n");
    
    // Geçici WAV dosyasını sil
    DeleteFile(wav_filename);
    printf("Gecici dosya silindi.\n");
    
    return 0;
}

