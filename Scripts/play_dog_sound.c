// Windows'ta dog_mp3_data.c dosyasından MP3 sesini oynatma programı
// GCC ile derleme: gcc -o play_dog_sound play_dog_sound.c dog_mp3_data.c -lwinmm
// Çalıştırma: play_dog_sound.exe

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

// Ses verisi için yapı
typedef struct {
    int16_t *pcm_data;
    size_t pcm_size;
    int sample_rate;
    int channels;
} AudioData;

// MP3'ü decode etme fonksiyonu
int decode_mp3(const uint8_t *mp3_data, size_t mp3_size, AudioData *audio) {
    mp3dec_t mp3d;
    mp3dec_frame_info_t info;
    
    // PCM verisi için büyük bir buffer ayır
    size_t max_samples = mp3_size * 10; // Yaklaşık tahmin
    int16_t *pcm_buffer = (int16_t *)malloc(max_samples * sizeof(int16_t));
    if (!pcm_buffer) {
        printf("Hata: Bellek ayrilamadi!\n");
        return 0;
    }
    
    mp3dec_init(&mp3d);
    
    size_t total_samples = 0;
    size_t offset = 0;
    int first_frame = 1;
    
    printf("MP3 decode ediliyor...\n");
    
    while (offset < mp3_size) {
        int16_t frame_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        int samples = mp3dec_decode_frame(&mp3d, mp3_data + offset, 
                                         mp3_size - offset, frame_pcm, &info);
        
        if (samples == 0 || info.frame_bytes == 0) {
            break;
        }
        
        // İlk frame'den sample rate ve channel bilgilerini al
        if (first_frame) {
            audio->sample_rate = info.hz;
            audio->channels = info.channels;
            printf("Sample Rate: %d Hz, Channels: %d\n", info.hz, info.channels);
            first_frame = 0;
        }
        
        // PCM verisini kopyala
        if (total_samples + samples > max_samples) {
            printf("Hata: PCM buffer dolu!\n");
            break;
        }
        
        memcpy(pcm_buffer + total_samples, frame_pcm, samples * sizeof(int16_t));
        total_samples += samples;
        offset += info.frame_bytes;
    }
    
    printf("Toplam %zu sample decode edildi (%.2f saniye)\n", 
           total_samples, 
           (float)total_samples / audio->sample_rate / audio->channels);
    
    audio->pcm_data = pcm_buffer;
    audio->pcm_size = total_samples * sizeof(int16_t);
    
    return 1;
}

// Windows Multimedia API kullanarak ses oynatma - chunked streaming
void play_audio(AudioData *audio) {
    HWAVEOUT hWaveOut;
    WAVEFORMATEX wfx;
    
    // WAVE formatını ayarla
    memset(&wfx, 0, sizeof(WAVEFORMATEX));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = audio->channels;
    wfx.nSamplesPerSec = audio->sample_rate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
    
    printf("\nSes formati:\n");
    printf("  Channels: %d\n", wfx.nChannels);
    printf("  Sample Rate: %d Hz\n", wfx.nSamplesPerSec);
    printf("  Bits Per Sample: %d\n", wfx.wBitsPerSample);
    printf("  Block Align: %d\n", wfx.nBlockAlign);
    printf("  Avg Bytes/Sec: %d\n\n", wfx.nAvgBytesPerSec);
    
    // Ses cihazını aç
    MMRESULT result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        printf("Hata: Ses cihazi acilamadi! (Kod: %d)\n", result);
        return;
    }
    
    printf("Ses oynatiliyor...\n");
    
    // Buffer'ı küçük parçalara böl (0.5 saniyelik chunk'lar)
    size_t chunk_size = wfx.nAvgBytesPerSec / 2; // 0.5 saniye
    size_t num_chunks = (audio->pcm_size + chunk_size - 1) / chunk_size;
    
    printf("Toplam %zu chunk oynatilacak (her biri ~0.5 saniye)\n", num_chunks);
    
    WAVEHDR *waveHdrs = (WAVEHDR *)calloc(num_chunks, sizeof(WAVEHDR));
    if (!waveHdrs) {
        printf("Hata: Bellek ayrilamadi!\n");
        waveOutClose(hWaveOut);
        return;
    }
    
    // Her chunk için header hazırla ve oynat
    for (size_t i = 0; i < num_chunks; i++) {
        size_t offset = i * chunk_size;
        size_t current_chunk_size = (offset + chunk_size > audio->pcm_size) 
                                    ? (audio->pcm_size - offset) 
                                    : chunk_size;
        
        // Wave header'ı hazırla
        waveHdrs[i].lpData = (LPSTR)(audio->pcm_data + offset);
        waveHdrs[i].dwBufferLength = current_chunk_size;
        waveHdrs[i].dwFlags = 0;
        waveHdrs[i].dwLoops = 0;
        
        // Buffer'ı hazırla
        result = waveOutPrepareHeader(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            printf("Hata: Chunk %zu hazirlanamaidi!\n", i);
            continue;
        }
        
        // Chunk'ı oynat
        result = waveOutWrite(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            printf("Hata: Chunk %zu oynatılamadi!\n", i);
            waveOutUnprepareHeader(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
            continue;
        }
        
        // İlk chunk başladıktan sonra hızlıca diğerlerini sıraya koy
        if (i == 0) {
            Sleep(50); // İlk chunk'ın başlaması için kısa bir bekleme
        }
    }
    
    printf("Tum chunk'lar sıraya kondu, oynatma devam ediyor...\n");
    
    // Tüm chunk'ların bitmesini bekle
    int all_done = 0;
    while (!all_done) {
        all_done = 1;
        for (size_t i = 0; i < num_chunks; i++) {
            if (!(waveHdrs[i].dwFlags & WHDR_DONE)) {
                all_done = 0;
                break;
            }
        }
        if (!all_done) {
            Sleep(50);
        }
    }
    
    printf("Ses oynatma tamamlandi!\n");
    
    // Temizlik
    for (size_t i = 0; i < num_chunks; i++) {
        if (waveHdrs[i].dwFlags & WHDR_PREPARED) {
            waveOutUnprepareHeader(hWaveOut, &waveHdrs[i], sizeof(WAVEHDR));
        }
    }
    
    free(waveHdrs);
    waveOutClose(hWaveOut);
}

int main() {
    printf("=== Dog MP3 Oynatici ===\n");
    printf("MP3 Dosya Boyutu: %u bytes\n\n", dog_mp3_file_size);
    
    AudioData audio = {0};
    
    // MP3'ü decode et
    if (!decode_mp3(dog_mp3_file_data, dog_mp3_file_size, &audio)) {
        printf("Hata: MP3 decode edilemedi!\n");
        return 1;
    }
    
    // Sesi oynat
    play_audio(&audio);
    
    // Belleği temizle
    if (audio.pcm_data) {
        free(audio.pcm_data);
    }
    
    printf("\nCikis icin bir tusa basin...\n");
    getchar();
    
    return 0;
}

