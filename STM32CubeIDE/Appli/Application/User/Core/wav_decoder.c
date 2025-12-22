/**
  ******************************************************************************
  * @file    wav_decoder.c
  * @brief   WAV file parser library implementation
  ******************************************************************************
  */

#include "wav_decoder.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* WAV File Format Structures */
#pragma pack(push, 1)

typedef struct {
    uint8_t  riff_header[4];        /* "RIFF" */
    uint32_t chunk_size;            /* File size - 8 */
    uint8_t  wave_header[4];        /* "WAVE" */
} WAV_RiffHeader_t;

typedef struct {
    uint8_t  chunk_id[4];           /* "fmt " */
    uint32_t chunk_size;            /* Size of format chunk (16, 18, or 40) */
    uint16_t audio_format;          /* Audio format (1 = PCM) */
    uint16_t num_channels;          /* Number of channels */
    uint32_t sample_rate;           /* Sample rate */
    uint32_t byte_rate;             /* Byte rate */
    uint16_t block_align;           /* Block align */
    uint16_t bits_per_sample;       /* Bits per sample */
} WAV_FormatHeader_t;

typedef struct {
    uint8_t  chunk_id[4];           /* "data" */
    uint32_t chunk_size;            /* Size of data */
} WAV_DataHeader_t;

#pragma pack(pop)

/* Private Function Prototypes */
static WAV_StatusTypeDef WAV_ParseRiffHeader(const uint8_t *file_data, WAV_RiffHeader_t *riff_header);
static WAV_StatusTypeDef WAV_ParseFormatChunk(const uint8_t *file_data, uint32_t *offset, WAV_FileInfo_t *wav_info);
static WAV_StatusTypeDef WAV_ParseDataChunk(const uint8_t *file_data, uint32_t file_size, uint32_t *offset, WAV_FileInfo_t *wav_info);
static bool WAV_FindChunk(const uint8_t *file_data, uint32_t file_size, uint32_t *offset, const char *chunk_id);

/**
  * @brief  Parse WAV file from memory address
  */
WAV_StatusTypeDef WAV_ParseFile(const uint8_t *file_address, uint32_t file_size, WAV_FileInfo_t *wav_info)
{
    WAV_StatusTypeDef status;
    WAV_RiffHeader_t riff_header;
    uint32_t offset = 0;
    
    /* Check parameters */
    if (file_address == NULL || wav_info == NULL) {
        return WAV_ERROR_NULL_POINTER;
    }
    
    if (file_size < sizeof(WAV_RiffHeader_t)) {
        return WAV_ERROR_INVALID_FILE;
    }
    
    /* Initialize WAV info structure */
    memset(wav_info, 0, sizeof(WAV_FileInfo_t));
    
    /* Parse RIFF header */
    status = WAV_ParseRiffHeader(file_address, &riff_header);
    if (status != WAV_OK) {
        return status;
    }
    
    wav_info->riff_chunk_size = riff_header.chunk_size;
    offset = sizeof(WAV_RiffHeader_t);
    
    /* Find and parse format chunk */
    if (!WAV_FindChunk(file_address, file_size, &offset, "fmt ")) {
        return WAV_ERROR_INVALID_FORMAT;
    }
    
    status = WAV_ParseFormatChunk(file_address, &offset, wav_info);
    if (status != WAV_OK) {
        return status;
    }
    
    /* Find and parse data chunk */
    if (!WAV_FindChunk(file_address, file_size, &offset, "data")) {
        return WAV_ERROR_NO_DATA_CHUNK;
    }
    
    status = WAV_ParseDataChunk(file_address, file_size, &offset, wav_info);
    if (status != WAV_OK) {
        return status;
    }
    
    /* Calculate additional information */
    if (wav_info->sample_rate > 0) {
        wav_info->num_samples = wav_info->data_size / (wav_info->num_channels * (wav_info->bits_per_sample / 8));
        wav_info->duration_ms = (wav_info->num_samples * 1000) / wav_info->sample_rate;
    }
    
    return WAV_OK;
}

/**
  * @brief  Parse RIFF header
  */
static WAV_StatusTypeDef WAV_ParseRiffHeader(const uint8_t *file_data, WAV_RiffHeader_t *riff_header)
{
    memcpy(riff_header, file_data, sizeof(WAV_RiffHeader_t));
    
    /* Check RIFF identifier */
    if (memcmp(riff_header->riff_header, "RIFF", 4) != 0) {
        return WAV_ERROR_INVALID_RIFF;
    }
    
    /* Check WAVE identifier */
    if (memcmp(riff_header->wave_header, "WAVE", 4) != 0) {
        return WAV_ERROR_INVALID_RIFF;
    }
    
    return WAV_OK;
}

/**
  * @brief  Parse format chunk
  */
static WAV_StatusTypeDef WAV_ParseFormatChunk(const uint8_t *file_data, uint32_t *offset, WAV_FileInfo_t *wav_info)
{
    WAV_FormatHeader_t format_header;
    
    /* Read format header */
    memcpy(&format_header, file_data + *offset, sizeof(WAV_FormatHeader_t));
    
    /* Validate format chunk */
    if (memcmp(format_header.chunk_id, "fmt ", 4) != 0) {
        return WAV_ERROR_INVALID_FORMAT;
    }
    
    /* Check if format is PCM or supported */
    if (format_header.audio_format != 1 && format_header.audio_format != 3) {
        /* 1 = PCM, 3 = IEEE float */
        return WAV_ERROR_UNSUPPORTED_FORMAT;
    }
    
    /* Fill WAV info structure */
    wav_info->audio_format = format_header.audio_format;
    wav_info->num_channels = format_header.num_channels;
    wav_info->sample_rate = format_header.sample_rate;
    wav_info->byte_rate = format_header.byte_rate;
    wav_info->block_align = format_header.block_align;
    wav_info->bits_per_sample = format_header.bits_per_sample;
    
    /* Move offset past format chunk */
    *offset += 8 + format_header.chunk_size;  /* chunk_id + chunk_size + data */
    
    return WAV_OK;
}

/**
  * @brief  Parse data chunk
  */
static WAV_StatusTypeDef WAV_ParseDataChunk(const uint8_t *file_data, uint32_t file_size, uint32_t *offset, WAV_FileInfo_t *wav_info)
{
    WAV_DataHeader_t data_header;
    
    /* Read data header */
    memcpy(&data_header, file_data + *offset, sizeof(WAV_DataHeader_t));
    
    /* Validate data chunk */
    if (memcmp(data_header.chunk_id, "data", 4) != 0) {
        return WAV_ERROR_NO_DATA_CHUNK;
    }
    
    wav_info->data_size = data_header.chunk_size;
    wav_info->data_offset = *offset + sizeof(WAV_DataHeader_t);
    
    /* Check if data size is valid */
    if (wav_info->data_offset + wav_info->data_size > file_size) {
        return WAV_ERROR_INVALID_FILE;
    }
    
    /* Allocate and copy audio data based on bit depth */
    if (wav_info->bits_per_sample == 16) {
        wav_info->raw_data = (int16_t *)malloc(wav_info->data_size);
        if (wav_info->raw_data == NULL) {
            return WAV_ERROR_ALLOCATION_FAILED;
        }
        memcpy(wav_info->raw_data, file_data + wav_info->data_offset, wav_info->data_size);
    } else if (wav_info->bits_per_sample == 8) {
        wav_info->raw_data_8bit = (uint8_t *)malloc(wav_info->data_size);
        if (wav_info->raw_data_8bit == NULL) {
            return WAV_ERROR_ALLOCATION_FAILED;
        }
        memcpy(wav_info->raw_data_8bit, file_data + wav_info->data_offset, wav_info->data_size);
    }
    
    return WAV_OK;
}

/**
  * @brief  Find a chunk in WAV file
  */
static bool WAV_FindChunk(const uint8_t *file_data, uint32_t file_size, uint32_t *offset, const char *chunk_id)
{
    uint8_t chunk_header[4];
    uint32_t chunk_size;
    
    while (*offset < file_size - 8) {
        /* Read chunk ID */
        memcpy(chunk_header, file_data + *offset, 4);
        
        /* Check if this is the chunk we're looking for */
        if (memcmp(chunk_header, chunk_id, 4) == 0) {
            return true;
        }
        
        /* Read chunk size and skip to next chunk */
        memcpy(&chunk_size, file_data + *offset + 4, 4);
        *offset += 8 + chunk_size;
        
        /* Ensure word alignment */
        if (chunk_size % 2 != 0) {
            *offset += 1;
        }
    }
    
    return false;
}

/**
  * @brief  Free allocated memory for WAV data
  */
void WAV_FreeData(WAV_FileInfo_t *wav_info)
{
    if (wav_info != NULL) {
        if (wav_info->raw_data != NULL) {
            free(wav_info->raw_data);
            wav_info->raw_data = NULL;
        }
        if (wav_info->raw_data_8bit != NULL) {
            free(wav_info->raw_data_8bit);
            wav_info->raw_data_8bit = NULL;
        }
    }
}

/**
  * @brief  Get audio data as 16-bit samples
  */
int16_t WAV_GetSample16(WAV_FileInfo_t *wav_info, uint32_t sample_index, uint8_t channel)
{
    if (wav_info == NULL || wav_info->raw_data == NULL) {
        return 0;
    }
    
    if (channel >= wav_info->num_channels) {
        return 0;
    }
    
    if (sample_index >= wav_info->num_samples) {
        return 0;
    }
    
    uint32_t index = sample_index * wav_info->num_channels + channel;
    return wav_info->raw_data[index];
}

/**
  * @brief  Check if WAV file is valid
  */
bool WAV_IsValid(WAV_FileInfo_t *wav_info)
{
    if (wav_info == NULL) {
        return false;
    }
    
    if (wav_info->sample_rate == 0 || wav_info->num_channels == 0) {
        return false;
    }
    
    if (wav_info->bits_per_sample != 8 && wav_info->bits_per_sample != 16 && 
        wav_info->bits_per_sample != 24 && wav_info->bits_per_sample != 32) {
        return false;
    }
    
    if (wav_info->data_size == 0) {
        return false;
    }
    
    if (wav_info->bits_per_sample == 16 && wav_info->raw_data == NULL) {
        return false;
    }
    
    if (wav_info->bits_per_sample == 8 && wav_info->raw_data_8bit == NULL) {
        return false;
    }
    
    return true;
}

/**
  * @brief  Get WAV file information as string
  */
void WAV_GetInfoString(WAV_FileInfo_t *wav_info, char *buffer, uint32_t buffer_size)
{
    if (wav_info == NULL || buffer == NULL) {
        return;
    }
    
    snprintf(buffer, buffer_size,
        "WAV File Information:\n"
        "Sample Rate: %lu Hz\n"
        "Channels: %u\n"
        "Bits Per Sample: %u\n"
        "Duration: %lu ms\n"
        "Data Size: %lu bytes\n"
        "Total Samples: %lu\n"
        "Audio Format: %s\n",
        (unsigned long)wav_info->sample_rate,
        wav_info->num_channels,
        wav_info->bits_per_sample,
        (unsigned long)wav_info->duration_ms,
        (unsigned long)wav_info->data_size,
        (unsigned long)wav_info->num_samples,
        (wav_info->audio_format == 1) ? "PCM" : "Unknown"
    );
}

