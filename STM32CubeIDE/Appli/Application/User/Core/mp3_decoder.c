// mp3_decoder.c - Implementation
#include "mp3_decoder.h"
#include <string.h>

/* Helper: Decode MP3 frames to fill chunk */
static int decode_to_fill_chunk(mp3_decoder_streaming_t *handle, 
                                int16_t *chunk_buffer, 
                                size_t chunk_size)
{
    size_t samples_written = 0;
    mp3dec_frame_info_t frame_info;
    int16_t temp_pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    
    while (samples_written < chunk_size) {
        /* Check if we reached end */
        if (handle->mp3_data_position >= handle->mp3_data_length) {
            if (handle->loop_enabled) {
                /* Loop back to start */
                handle->mp3_data_position = 0;
                mp3dec_init(&handle->decoder);
            } else {
                /* Fill rest with silence */
                memset(&chunk_buffer[samples_written], 0, 
                       (chunk_size - samples_written) * sizeof(int16_t));
                return samples_written > 0 ? MP3_DEC_OK : MP3_DEC_END_OF_FILE;
            }
        }
        
        /* Decode one frame */
        int samples = mp3dec_decode_frame(
            &handle->decoder,
            handle->mp3_data + handle->mp3_data_position,
            handle->mp3_data_length - handle->mp3_data_position,
            temp_pcm,
            &frame_info
        );
        
        if (samples == 0 || frame_info.frame_bytes == 0) {
            /* Skip invalid data */
            handle->mp3_data_position++;
            continue;
        }
        
        handle->mp3_data_position += frame_info.frame_bytes;
        handle->sample_rate = frame_info.hz;
        handle->channels = frame_info.channels;
        
        /* Convert to stereo if mono */
        int16_t *source = temp_pcm;
        size_t output_samples = samples * frame_info.channels;
        
        if (frame_info.channels == 1) {
            /* Mono to stereo */
            for (int i = samples - 1; i >= 0; i--) {
                temp_pcm[i * 2 + 1] = temp_pcm[i];
                temp_pcm[i * 2] = temp_pcm[i];
            }
            output_samples = samples * 2;
        }
        
        /* Copy to chunk buffer */
        size_t samples_to_copy = output_samples;
        if (samples_written + samples_to_copy > chunk_size) {
            samples_to_copy = chunk_size - samples_written;
        }
        
        memcpy(&chunk_buffer[samples_written], source, 
               samples_to_copy * sizeof(int16_t));
        samples_written += samples_to_copy;
    }
    
    return MP3_DEC_OK;
}

int mp3_decoder_streaming_init(mp3_decoder_streaming_t *handle,
                               int16_t *output_buffer,
                               size_t chunk_size)
{
    if (!handle || !output_buffer || chunk_size == 0) {
        return MP3_DEC_INVALID_PARAM;
    }
    
    memset(handle, 0, sizeof(mp3_decoder_streaming_t));
    
    mp3dec_init(&handle->decoder);
    
    handle->output_buffer = output_buffer;
    handle->output_buffer_size = chunk_size * 2; /* Ping-pong */
    handle->chunk_size = chunk_size;
    handle->buffer_index = 0;
    handle->loop_enabled = 0;
    
    return MP3_DEC_OK;
}

int mp3_decoder_streaming_load(mp3_decoder_streaming_t *handle,
                               const uint8_t *mp3_data,
                               size_t mp3_length)
{
    if (!handle || !mp3_data || mp3_length == 0) {
        return MP3_DEC_INVALID_PARAM;
    }
    
    handle->mp3_data = mp3_data;
    handle->mp3_data_length = mp3_length;
    handle->mp3_data_position = 0;
    handle->total_samples_decoded = 0;
    
    return MP3_DEC_OK;
}

int16_t* mp3_decoder_streaming_start(mp3_decoder_streaming_t *handle)
{
    if (!handle || !handle->mp3_data) {
        return NULL;
    }
    
    /* Decode first chunk (buffer 0) */
    int result = decode_to_fill_chunk(handle, handle->output_buffer, 
                                     handle->chunk_size);
    
    if (result != MP3_DEC_OK) {
        return NULL;
    }
    
    handle->current_chunk = handle->output_buffer;
    handle->buffer_index = 0;
    handle->samples_in_chunk = handle->chunk_size;
    
    return handle->current_chunk;
}

int16_t* mp3_decoder_streaming_next_chunk(mp3_decoder_streaming_t *handle,
                                          size_t *samples_decoded)
{
    if (!handle || !handle->output_buffer) {
        return NULL;
    }
    
    /* Switch buffer (ping-pong) */
    handle->buffer_index = (handle->buffer_index == 0) ? 1 : 0;
    int16_t *next_buffer = handle->output_buffer + 
                          (handle->buffer_index * handle->chunk_size);
    
    /* Decode next chunk */
    int result = decode_to_fill_chunk(handle, next_buffer, handle->chunk_size);
    
    if (result == MP3_DEC_END_OF_FILE && !handle->loop_enabled) {
        return NULL;
    }
    
    handle->current_chunk = next_buffer;
    handle->samples_in_chunk = handle->chunk_size;
    handle->total_samples_decoded += handle->chunk_size;
    
    if (samples_decoded) {
        *samples_decoded = handle->samples_in_chunk;
    }
    
    return handle->current_chunk;
}

int mp3_decoder_streaming_reset(mp3_decoder_streaming_t *handle)
{
    if (!handle) {
        return MP3_DEC_INVALID_PARAM;
    }
    
    handle->mp3_data_position = 0;
    handle->total_samples_decoded = 0;
    handle->buffer_index = 0;
    mp3dec_init(&handle->decoder);
    
    return MP3_DEC_OK;
}

void mp3_decoder_streaming_set_loop(mp3_decoder_streaming_t *handle, uint8_t enable)
{
    if (handle) {
        handle->loop_enabled = enable;
    }
}