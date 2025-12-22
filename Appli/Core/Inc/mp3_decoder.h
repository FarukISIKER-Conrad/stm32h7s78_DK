// mp3_decoder.h - Streaming API için güncellenmiş header
#ifndef __MP3_DECODER_H
#define __MP3_DECODER_H

#include <stdint.h>
#include <stddef.h>
#include "minimp3.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Return codes */
#define MP3_DEC_OK                  0
#define MP3_DEC_ERROR              -1
#define MP3_DEC_NEED_MORE_DATA     -2
#define MP3_DEC_END_OF_FILE        -3
#define MP3_DEC_INVALID_PARAM      -4

/* Configuration */
#define MP3_TARGET_SAMPLE_RATE     48000
#define MP3_OUTPUT_CHANNELS        2

/* Streaming decoder handle */
typedef struct {
    mp3dec_t decoder;                    /* minimp3 decoder */
    
    const uint8_t *mp3_data;             /* MP3 source data */
    size_t mp3_data_length;              /* Total MP3 data length */
    size_t mp3_data_position;            /* Current read position */
    
    int16_t *output_buffer;              /* Output PCM buffer (ping-pong) */
    size_t output_buffer_size;           /* Size in samples */
    
    int16_t *current_chunk;              /* Current chunk pointer for DMA */
    size_t chunk_size;                   /* Chunk size in samples */
    size_t samples_in_chunk;             /* Valid samples in current chunk */
    
    uint8_t buffer_index;                /* 0 or 1 for ping-pong */
    uint8_t loop_enabled;                /* Loop playback */
    
    uint32_t sample_rate;
    uint8_t channels;
    uint32_t total_samples_decoded;
} mp3_decoder_streaming_t;

/**
 * @brief Initialize streaming decoder
 * @param handle Decoder handle
 * @param output_buffer Output buffer (2x chunk_size for ping-pong)
 * @param chunk_size Size of each chunk in samples (stereo pairs * 2)
 * @return MP3_DEC_OK on success
 */
int mp3_decoder_streaming_init(mp3_decoder_streaming_t *handle, 
                               int16_t *output_buffer, 
                               size_t chunk_size);

/**
 * @brief Load MP3 data
 * @param handle Decoder handle
 * @param mp3_data MP3 data pointer
 * @param mp3_length MP3 data length
 * @return MP3_DEC_OK on success
 */
int mp3_decoder_streaming_load(mp3_decoder_streaming_t *handle,
                               const uint8_t *mp3_data,
                               size_t mp3_length);

/**
 * @brief Start streaming - decode first chunk
 * @param handle Decoder handle
 * @return Pointer to first chunk (int16_t*) or NULL on error
 */
int16_t* mp3_decoder_streaming_start(mp3_decoder_streaming_t *handle);

/**
 * @brief Get next chunk for DMA (decode next frames)
 * Call this from DMA half/complete callbacks
 * @param handle Decoder handle
 * @param samples_decoded Number of samples decoded (can be NULL)
 * @return Pointer to next chunk (int16_t*) or NULL if end of file
 */
int16_t* mp3_decoder_streaming_next_chunk(mp3_decoder_streaming_t *handle,
                                          size_t *samples_decoded);

/**
 * @brief Reset to beginning
 * @param handle Decoder handle
 * @return MP3_DEC_OK on success
 */
int mp3_decoder_streaming_reset(mp3_decoder_streaming_t *handle);

/**
 * @brief Enable/disable loop playback
 * @param handle Decoder handle
 * @param enable 1 to enable, 0 to disable
 */
void mp3_decoder_streaming_set_loop(mp3_decoder_streaming_t *handle, uint8_t enable);

#ifdef __cplusplus
}
#endif

#endif /* __MP3_DECODER_H */