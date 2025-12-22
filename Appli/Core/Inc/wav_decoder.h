/**
  ******************************************************************************
  * @file    wav_decoder.h
  * @brief   WAV file parser library header
  ******************************************************************************
  */

#ifndef WAV_DECODER_H
#define WAV_DECODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* WAV Parser Return Codes */
typedef enum {
    WAV_OK = 0,
    WAV_ERROR_INVALID_FILE,
    WAV_ERROR_INVALID_RIFF,
    WAV_ERROR_INVALID_FORMAT,
    WAV_ERROR_UNSUPPORTED_FORMAT,
    WAV_ERROR_NO_DATA_CHUNK,
    WAV_ERROR_NULL_POINTER,
    WAV_ERROR_ALLOCATION_FAILED
} WAV_StatusTypeDef;

/* WAV File Format Information */
typedef struct {
    /* RIFF Chunk Descriptor */
    uint32_t riff_chunk_size;      /* File size - 8 bytes */
    
    /* Format Chunk */
    uint16_t audio_format;          /* Audio format (1 = PCM) */
    uint16_t num_channels;          /* Number of channels (1=Mono, 2=Stereo) */
    uint32_t sample_rate;           /* Sample rate (Hz) */
    uint32_t byte_rate;             /* Byte rate (SampleRate * NumChannels * BitsPerSample/8) */
    uint16_t block_align;           /* Block align (NumChannels * BitsPerSample/8) */
    uint16_t bits_per_sample;       /* Bits per sample (8, 16, 24, 32) */
    
    /* Data Chunk */
    uint32_t data_size;             /* Size of audio data in bytes */
    uint32_t num_samples;           /* Total number of samples (per channel) */
    
    /* Audio Data */
    int16_t *raw_data;              /* Pointer to raw audio data (16-bit samples) */
    uint8_t *raw_data_8bit;         /* Pointer to raw audio data (8-bit samples) */
    
    /* File Info */
    uint32_t duration_ms;           /* Duration in milliseconds */
    uint32_t data_offset;           /* Offset to data chunk in file */
} WAV_FileInfo_t;

/* Function Prototypes */

/**
  * @brief  Parse WAV file from memory address
  * @param  file_address: Pointer to WAV file in memory
  * @param  file_size: Size of the WAV file in bytes
  * @param  wav_info: Pointer to WAV_FileInfo_t structure to fill
  * @retval WAV_StatusTypeDef: Status of the operation
  */
WAV_StatusTypeDef WAV_ParseFile(const uint8_t *file_address, uint32_t file_size, WAV_FileInfo_t *wav_info);

/**
  * @brief  Free allocated memory for WAV data
  * @param  wav_info: Pointer to WAV_FileInfo_t structure
  * @retval None
  */
void WAV_FreeData(WAV_FileInfo_t *wav_info);

/**
  * @brief  Get audio data as 16-bit samples
  * @param  wav_info: Pointer to WAV_FileInfo_t structure
  * @param  sample_index: Sample index to read
  * @param  channel: Channel number (0 for left, 1 for right in stereo)
  * @retval int16_t: Audio sample value
  */
int16_t WAV_GetSample16(WAV_FileInfo_t *wav_info, uint32_t sample_index, uint8_t channel);

/**
  * @brief  Check if WAV file is valid
  * @param  wav_info: Pointer to WAV_FileInfo_t structure
  * @retval bool: true if valid, false otherwise
  */
bool WAV_IsValid(WAV_FileInfo_t *wav_info);

/**
  * @brief  Get WAV file information as string
  * @param  wav_info: Pointer to WAV_FileInfo_t structure
  * @param  buffer: Buffer to store the string
  * @param  buffer_size: Size of the buffer
  * @retval None
  */
void WAV_GetInfoString(WAV_FileInfo_t *wav_info, char *buffer, uint32_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* WAV_DECODER_H */

