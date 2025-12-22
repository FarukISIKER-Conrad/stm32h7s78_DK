/**
  ******************************************************************************
  * @file    wav_decoder_example.c
  * @brief   WAV decoder library usage example
  ******************************************************************************
  */

#include "wav_decoder.h"
#include <stdio.h>

/* Example WAV file in memory (you would load this from file system or external memory) */
extern uint8_t wav_file_data[];
extern uint32_t wav_file_size;

/**
  * @brief  Example: Parse and play a WAV file
  */
void WAV_Example_ParseAndPlay(void)
{
    WAV_FileInfo_t wav_info;
    WAV_StatusTypeDef status;
    char info_buffer[256];
    
    /* Parse WAV file from memory */
    status = WAV_ParseFile(wav_file_data, wav_file_size, &wav_info);
    
    if (status != WAV_OK) {
        printf("Error parsing WAV file: %d\n", status);
        return;
    }
    
    /* Check if file is valid */
    if (!WAV_IsValid(&wav_info)) {
        printf("Invalid WAV file\n");
        WAV_FreeData(&wav_info);
        return;
    }
    
    /* Print WAV file information */
    WAV_GetInfoString(&wav_info, info_buffer, sizeof(info_buffer));
    printf("%s\n", info_buffer);
    
    /* Access raw audio data for playback */
    if (wav_info.bits_per_sample == 16) {
        /* Play 16-bit audio */
        int16_t *audio_data = wav_info.raw_data;
        uint32_t num_samples = wav_info.num_samples;
        
        printf("Playing %lu samples at %lu Hz...\n", 
               (unsigned long)num_samples, 
               (unsigned long)wav_info.sample_rate);
        
        /* Example: Send to audio codec/DAC */
        // for (uint32_t i = 0; i < num_samples; i++) {
        //     if (wav_info.num_channels == 2) {
        //         int16_t left = audio_data[i * 2];
        //         int16_t right = audio_data[i * 2 + 1];
        //         // Send to stereo DAC
        //     } else {
        //         int16_t sample = audio_data[i];
        //         // Send to mono DAC
        //     }
        // }
    } else if (wav_info.bits_per_sample == 8) {
        /* Play 8-bit audio */
        uint8_t *audio_data = wav_info.raw_data_8bit;
        uint32_t num_samples = wav_info.num_samples;
        
        printf("Playing %lu samples at %lu Hz...\n", 
               (unsigned long)num_samples, 
               (unsigned long)wav_info.sample_rate);
    }
    
    /* Free allocated memory when done */
    WAV_FreeData(&wav_info);
}

/**
  * @brief  Example: Read individual samples
  */
void WAV_Example_ReadSamples(void)
{
    WAV_FileInfo_t wav_info;
    WAV_StatusTypeDef status;
    
    /* Parse WAV file */
    status = WAV_ParseFile(wav_file_data, wav_file_size, &wav_info);
    
    if (status == WAV_OK && WAV_IsValid(&wav_info)) {
        /* Read first 10 samples from left channel */
        for (uint32_t i = 0; i < 10 && i < wav_info.num_samples; i++) {
            int16_t sample = WAV_GetSample16(&wav_info, i, 0);  /* Channel 0 = left */
            printf("Sample %lu: %d\n", (unsigned long)i, sample);
        }
        
        /* If stereo, read from right channel */
        if (wav_info.num_channels == 2) {
            for (uint32_t i = 0; i < 10 && i < wav_info.num_samples; i++) {
                int16_t sample = WAV_GetSample16(&wav_info, i, 1);  /* Channel 1 = right */
                printf("Sample %lu (Right): %d\n", (unsigned long)i, sample);
            }
        }
    }
    
    WAV_FreeData(&wav_info);
}

/**
  * @brief  Example: Use with DMA for audio playback
  */
void WAV_Example_DMAPlayback(void)
{
    WAV_FileInfo_t wav_info;
    WAV_StatusTypeDef status;
    
    /* Parse WAV file */
    status = WAV_ParseFile(wav_file_data, wav_file_size, &wav_info);
    
    if (status == WAV_OK && WAV_IsValid(&wav_info)) {
        /* Configure audio codec for this sample rate and format */
        // Audio_Init(wav_info.sample_rate, wav_info.num_channels, wav_info.bits_per_sample);
        
        /* Start DMA transfer with raw audio data */
        if (wav_info.bits_per_sample == 16) {
            // HAL_I2S_Transmit_DMA(&hi2s, (uint16_t*)wav_info.raw_data, wav_info.data_size / 2);
            printf("Started DMA playback of %lu bytes\n", (unsigned long)wav_info.data_size);
        }
        
        /* Note: Don't call WAV_FreeData until playback is complete! */
        // Wait for playback to finish, then:
        // WAV_FreeData(&wav_info);
    }
}

/**
  * @brief  Example: Error handling
  */
void WAV_Example_ErrorHandling(void)
{
    WAV_FileInfo_t wav_info;
    WAV_StatusTypeDef status;
    
    status = WAV_ParseFile(wav_file_data, wav_file_size, &wav_info);
    
    switch (status) {
        case WAV_OK:
            printf("WAV file parsed successfully\n");
            WAV_FreeData(&wav_info);
            break;
            
        case WAV_ERROR_INVALID_FILE:
            printf("Error: Invalid file\n");
            break;
            
        case WAV_ERROR_INVALID_RIFF:
            printf("Error: Not a valid RIFF/WAVE file\n");
            break;
            
        case WAV_ERROR_INVALID_FORMAT:
            printf("Error: Invalid format chunk\n");
            break;
            
        case WAV_ERROR_UNSUPPORTED_FORMAT:
            printf("Error: Unsupported audio format (only PCM supported)\n");
            break;
            
        case WAV_ERROR_NO_DATA_CHUNK:
            printf("Error: No data chunk found\n");
            break;
            
        case WAV_ERROR_NULL_POINTER:
            printf("Error: NULL pointer passed\n");
            break;
            
        case WAV_ERROR_ALLOCATION_FAILED:
            printf("Error: Memory allocation failed\n");
            break;
            
        default:
            printf("Error: Unknown error\n");
            break;
    }
}

