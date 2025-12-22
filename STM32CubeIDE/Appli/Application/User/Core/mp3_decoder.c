/*
 * mp3_decoder.c
 *
 * MP3 Decoder Library Implementation
 * Supports 48kHz sampling rate with 16-bit stereo output
 *
 * Created on: Dec 22, 2025
 * Author: Faruk Isıker
 */

#include "mp3_decoder.h"
#include <string.h>
#include <stdlib.h>

/* Stub implementation for minimp3 - you'll need to include the full minimp3 implementation */
/* For now, this provides the API structure */

void mp3dec_init(mp3dec_t *dec)
{
    if (dec) {
        memset(dec, 0, sizeof(mp3dec_t));
    }
}

int mp3dec_decode_frame(mp3dec_t *dec, const uint8_t *mp3, int mp3_bytes, 
                       int16_t *pcm, mp3dec_frame_info_t *info)
{
    /* This is a stub - full minimp3 implementation needed */
    /* Returns number of samples decoded */
    if (!dec || !mp3 || !pcm || !info) {
        return 0;
    }
    
    /* Stub: Return 0 samples for now */
    info->frame_bytes = 0;
    info->channels = 2;
    info->hz = 48000;
    info->layer = 3;
    info->bitrate_kbps = 128;
    
    return 0;
}

/* MP3 Decoder Implementation */

int mp3_decoder_init(mp3_decoder_handle_t *handle)
{
    if (!handle) {
        return MP3_DEC_INVALID_PARAM;
    }
    
    memset(handle, 0, sizeof(mp3_decoder_handle_t));
    
    mp3dec_init(&handle->decoder);
    
    handle->state = MP3_STATE_IDLE;
    handle->sample_rate = MP3_TARGET_SAMPLE_RATE;
    handle->channels = MP3_OUTPUT_CHANNELS;
    handle->resampling_enabled = 1;
    
    return MP3_DEC_OK;
}

int mp3_decoder_load(mp3_decoder_handle_t *handle, const uint8_t *mp3_data, size_t mp3_length)
{
    if (!handle || !mp3_data || mp3_length == 0) {
        return MP3_DEC_INVALID_PARAM;
    }
    
    handle->mp3_data = mp3_data;
    handle->mp3_data_length = mp3_length;
    handle->mp3_data_position = 0;
    handle->total_samples_decoded = 0;
    handle->state = MP3_STATE_STOPPED;
    
    return MP3_DEC_OK;
}

int mp3_decoder_set_output_buffer(mp3_decoder_handle_t *handle, int16_t *output_buffer, size_t buffer_size)
{
    if (!handle || !output_buffer || buffer_size == 0) {
        return MP3_DEC_INVALID_PARAM;
    }
    
    handle->output_buffer = output_buffer;
    handle->output_buffer_size = buffer_size;
    handle->output_buffer_position = 0;
    
    return MP3_DEC_OK;
}

int mp3_decoder_decode_frame(mp3_decoder_handle_t *handle, size_t *samples_decoded)
{
    if (!handle || !handle->mp3_data || !handle->output_buffer) {
        return MP3_DEC_INVALID_PARAM;
    }
    
    if (handle->mp3_data_position >= handle->mp3_data_length) {
        handle->state = MP3_STATE_STOPPED;
        return MP3_DEC_END_OF_FILE;
    }
    
    mp3dec_frame_info_t frame_info;
    int16_t pcm_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME];
    
    /* Decode one MP3 frame */
    int samples = mp3dec_decode_frame(
        &handle->decoder,
        handle->mp3_data + handle->mp3_data_position,
        handle->mp3_data_length - handle->mp3_data_position,
        pcm_buffer,
        &frame_info
    );
    
    if (samples <= 0 || frame_info.frame_bytes == 0) {
        if (handle->mp3_data_position >= handle->mp3_data_length) {
            return MP3_DEC_END_OF_FILE;
        }
        /* Skip invalid frame */
        handle->mp3_data_position++;
        return MP3_DEC_NEED_MORE_DATA;
    }
    
    /* Update position */
    handle->mp3_data_position += frame_info.frame_bytes;
    
    /* Update decoder info */
    handle->sample_rate = frame_info.hz;
    handle->channels = frame_info.channels;
    handle->bitrate = frame_info.bitrate_kbps;
    
    /* Process decoded samples */
    size_t output_samples = samples;
    int16_t *source = pcm_buffer;
    int16_t temp_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME * 2];
    
    /* Convert mono to stereo if needed */
    if (frame_info.channels == 1) {
        mp3_decoder_mono_to_stereo(pcm_buffer, temp_buffer, samples);
        source = temp_buffer;
        output_samples = samples * 2;
    }
    
    /* Resample if needed */
    if (handle->resampling_enabled && frame_info.hz != MP3_TARGET_SAMPLE_RATE) {
        int16_t resample_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME * 2];
        output_samples = mp3_decoder_resample(
            source,
            output_samples / 2,  /* Stereo pairs */
            frame_info.hz,
            resample_buffer,
            MINIMP3_MAX_SAMPLES_PER_FRAME,
            MP3_TARGET_SAMPLE_RATE
        );
        output_samples *= 2;  /* Convert back to total samples */
        source = resample_buffer;
    }
    
    /* Copy to output buffer */
    size_t samples_to_copy = output_samples;
    if (handle->output_buffer_position + samples_to_copy > handle->output_buffer_size) {
        samples_to_copy = handle->output_buffer_size - handle->output_buffer_position;
    }
    
    memcpy(&handle->output_buffer[handle->output_buffer_position], source, samples_to_copy * sizeof(int16_t));
    handle->output_buffer_position += samples_to_copy;
    handle->total_samples_decoded += samples_to_copy;
    
    if (samples_decoded) {
        *samples_decoded = samples_to_copy;
    }
    
    handle->state = MP3_STATE_PLAYING;
    
    return MP3_DEC_OK;
}

int mp3_decoder_process(mp3_decoder_handle_t *handle, int16_t *output_pcm, size_t output_size, size_t *samples_decoded)
{
    if (!handle || !output_pcm || output_size == 0) {
        return MP3_DEC_INVALID_PARAM;
    }
    
    /* Set output buffer */
    int result = mp3_decoder_set_output_buffer(handle, output_pcm, output_size);
    if (result != MP3_DEC_OK) {
        return result;
    }
    
    /* Reset position */
    handle->mp3_data_position = 0;
    handle->output_buffer_position = 0;
    handle->total_samples_decoded = 0;
    
    /* Decode all frames */
    while (handle->output_buffer_position < handle->output_buffer_size) {
        size_t frame_samples = 0;
        result = mp3_decoder_decode_frame(handle, &frame_samples);
        
        if (result == MP3_DEC_END_OF_FILE) {
            break;
        }
        
        if (result != MP3_DEC_OK && result != MP3_DEC_NEED_MORE_DATA) {
            handle->state = MP3_STATE_ERROR;
            return result;
        }
    }
    
    if (samples_decoded) {
        *samples_decoded = handle->output_buffer_position;
    }
    
    handle->state = MP3_STATE_STOPPED;
    
    return MP3_DEC_OK;
}

mp3_decoder_state_t mp3_decoder_get_state(mp3_decoder_handle_t *handle)
{
    if (!handle) {
        return MP3_STATE_ERROR;
    }
    return handle->state;
}

int mp3_decoder_reset(mp3_decoder_handle_t *handle)
{
    if (!handle) {
        return MP3_DEC_INVALID_PARAM;
    }
    
    handle->mp3_data_position = 0;
    handle->output_buffer_position = 0;
    handle->total_samples_decoded = 0;
    handle->state = MP3_STATE_STOPPED;
    
    mp3dec_init(&handle->decoder);
    
    return MP3_DEC_OK;
}

int mp3_decoder_get_info(mp3_decoder_handle_t *handle, uint32_t *sample_rate, uint8_t *channels, uint32_t *bitrate)
{
    if (!handle) {
        return MP3_DEC_INVALID_PARAM;
    }
    
    if (sample_rate) {
        *sample_rate = handle->sample_rate;
    }
    
    if (channels) {
        *channels = handle->channels;
    }
    
    if (bitrate) {
        *bitrate = handle->bitrate;
    }
    
    return MP3_DEC_OK;
}

void mp3_decoder_mono_to_stereo(const int16_t *input, int16_t *output, size_t samples)
{
    if (!input || !output || samples == 0) {
        return;
    }
    
    for (size_t i = 0; i < samples; i++) {
        output[i * 2] = input[i];      /* Left channel */
        output[i * 2 + 1] = input[i];  /* Right channel */
    }
}

size_t mp3_decoder_resample(const int16_t *input, size_t input_samples, uint32_t input_rate,
                           int16_t *output, size_t output_samples, uint32_t output_rate)
{
    if (!input || !output || input_samples == 0 || input_rate == 0 || output_rate == 0) {
        return 0;
    }
    
    /* Simple nearest-neighbor resampling */
    float ratio = (float)input_rate / (float)output_rate;
    size_t samples_generated = 0;
    
    for (size_t i = 0; i < output_samples && samples_generated < output_samples; i++) {
        size_t input_index = (size_t)(i * ratio);
        
        if (input_index >= input_samples) {
            break;
        }
        
        /* Copy stereo pair */
        output[samples_generated * 2] = input[input_index * 2];      /* Left */
        output[samples_generated * 2 + 1] = input[input_index * 2 + 1];  /* Right */
        samples_generated++;
    }
    
    return samples_generated;
}

