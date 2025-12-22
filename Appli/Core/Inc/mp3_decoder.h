/*
 * mp3_decoder.h
 *
 * MP3 Decoder Library for STM32
 * Supports 48kHz sampling rate with 16-bit stereo output
 *
 * Created on: Dec 22, 2025
 * Author: Faruk Isıker
 */

#ifndef __MP3_DECODER_H
#define __MP3_DECODER_H

#include <stdint.h>
#include <stddef.h>
#include "minimp3.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MP3 Decoder Return Codes */
#define MP3_DEC_OK                  0
#define MP3_DEC_ERROR              -1
#define MP3_DEC_NEED_MORE_DATA     -2
#define MP3_DEC_END_OF_FILE        -3
#define MP3_DEC_INVALID_PARAM      -4

/* MP3 Decoder Configuration */
#define MP3_TARGET_SAMPLE_RATE     48000
#define MP3_OUTPUT_CHANNELS        2
#define MP3_BITS_PER_SAMPLE        16

/* MP3 Decoder State */
typedef enum {
    MP3_STATE_IDLE = 0,
    MP3_STATE_PLAYING,
    MP3_STATE_PAUSED,
    MP3_STATE_STOPPED,
    MP3_STATE_ERROR
} mp3_decoder_state_t;

/* MP3 Decoder Handle Structure */
typedef struct {
    mp3dec_t decoder;                    /* minimp3 decoder instance */
    
    const uint8_t *mp3_data;             /* Pointer to MP3 file data */
    size_t mp3_data_length;              /* Total length of MP3 data */
    size_t mp3_data_position;            /* Current read position in MP3 data */
    
    int16_t *output_buffer;              /* Output PCM buffer */
    size_t output_buffer_size;           /* Size of output buffer in samples */
    size_t output_buffer_position;       /* Current write position in output buffer */
    
    mp3_decoder_state_t state;           /* Current decoder state */
    
    uint32_t sample_rate;                /* Current sample rate */
    uint8_t channels;                    /* Number of channels (1=mono, 2=stereo) */
    uint32_t bitrate;                    /* Current bitrate in kbps */
    
    uint32_t total_samples_decoded;      /* Total samples decoded */
    uint8_t resampling_enabled;          /* Enable/disable resampling to 48kHz */
} mp3_decoder_handle_t;

/**
 * @brief Initialize MP3 decoder
 * @param handle Pointer to MP3 decoder handle
 * @return MP3_DEC_OK on success, error code otherwise
 */
int mp3_decoder_init(mp3_decoder_handle_t *handle);

/**
 * @brief Load MP3 file data for decoding
 * @param handle Pointer to MP3 decoder handle
 * @param mp3_data Pointer to MP3 file data in memory
 * @param mp3_length Length of MP3 data in bytes
 * @return MP3_DEC_OK on success, error code otherwise
 */
int mp3_decoder_load(mp3_decoder_handle_t *handle, const uint8_t *mp3_data, size_t mp3_length);

/**
 * @brief Set output buffer for decoded PCM data
 * @param handle Pointer to MP3 decoder handle
 * @param output_buffer Pointer to int16_t buffer for PCM output
 * @param buffer_size Size of buffer in samples (not bytes)
 * @return MP3_DEC_OK on success, error code otherwise
 */
int mp3_decoder_set_output_buffer(mp3_decoder_handle_t *handle, int16_t *output_buffer, size_t buffer_size);

/**
 * @brief Decode next frame of MP3 data
 * @param handle Pointer to MP3 decoder handle
 * @param samples_decoded Pointer to store number of samples decoded (can be NULL)
 * @return MP3_DEC_OK on success, error code otherwise
 */
int mp3_decoder_decode_frame(mp3_decoder_handle_t *handle, size_t *samples_decoded);

/**
 * @brief Process entire MP3 file and fill output buffer
 * @param handle Pointer to MP3 decoder handle
 * @param output_pcm Output buffer for PCM data (int16_t stereo samples)
 * @param output_size Size of output buffer in samples
 * @param samples_decoded Pointer to store total samples decoded
 * @return MP3_DEC_OK on success, error code otherwise
 */
int mp3_decoder_process(mp3_decoder_handle_t *handle, int16_t *output_pcm, size_t output_size, size_t *samples_decoded);

/**
 * @brief Get current decoder state
 * @param handle Pointer to MP3 decoder handle
 * @return Current decoder state
 */
mp3_decoder_state_t mp3_decoder_get_state(mp3_decoder_handle_t *handle);

/**
 * @brief Reset decoder to beginning of file
 * @param handle Pointer to MP3 decoder handle
 * @return MP3_DEC_OK on success, error code otherwise
 */
int mp3_decoder_reset(mp3_decoder_handle_t *handle);

/**
 * @brief Get MP3 file information
 * @param handle Pointer to MP3 decoder handle
 * @param sample_rate Pointer to store sample rate (can be NULL)
 * @param channels Pointer to store number of channels (can be NULL)
 * @param bitrate Pointer to store bitrate in kbps (can be NULL)
 * @return MP3_DEC_OK on success, error code otherwise
 */
int mp3_decoder_get_info(mp3_decoder_handle_t *handle, uint32_t *sample_rate, uint8_t *channels, uint32_t *bitrate);

/**
 * @brief Convert mono to stereo
 * @param input Input mono buffer
 * @param output Output stereo buffer
 * @param samples Number of mono samples
 */
void mp3_decoder_mono_to_stereo(const int16_t *input, int16_t *output, size_t samples);

/**
 * @brief Simple resampling (nearest neighbor)
 * @param input Input buffer
 * @param input_samples Number of input samples (stereo pairs)
 * @param input_rate Input sample rate
 * @param output Output buffer
 * @param output_samples Maximum output samples
 * @param output_rate Output sample rate (48000)
 * @return Number of output samples generated
 */
size_t mp3_decoder_resample(const int16_t *input, size_t input_samples, uint32_t input_rate,
                           int16_t *output, size_t output_samples, uint32_t output_rate);

#ifdef __cplusplus
}
#endif

#endif /* __MP3_DECODER_H */

