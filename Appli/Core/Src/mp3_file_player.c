/**
  ******************************************************************************
  * @file    mp3_file_player.c
  * @brief   MP3 File Player Implementation
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "mp3_file_player.h"
#include <string.h>
#include <stdio.h>

/* Private functions ---------------------------------------------------------*/

/**
 * @brief  QSPI Flash'ten daha fazla MP3 verisi oku
 * @param  player: Player handle
 * @retval Bytes read, 0 if EOF, <0 if error
 */
static int mp3_file_read_more(mp3_file_player_t *player)
{
    FRESULT fres;
    UINT bytes_read;
    
    if (player->eof) {
        return 0;
    }
    
    // Kalan veriyi buffer başına taşı
    if (player->mp3_buffer_position < player->mp3_bytes_in_buffer) {
        size_t remaining = player->mp3_bytes_in_buffer - player->mp3_buffer_position;
        memmove(player->mp3_buffer, 
                player->mp3_buffer + player->mp3_buffer_position, 
                remaining);
        player->mp3_bytes_in_buffer = remaining;
    } else {
        player->mp3_bytes_in_buffer = 0;
    }
    
    player->mp3_buffer_position = 0;
    
    // QSPI Flash'ten yeni veri oku
    fres = f_read(&player->file, 
                  player->mp3_buffer + player->mp3_bytes_in_buffer,
                  MP3_READ_BUFFER_SIZE - player->mp3_bytes_in_buffer,
                  &bytes_read);
    
    if (fres != FR_OK) {
        return -1;
    }
    
    player->mp3_bytes_in_buffer += bytes_read;
    player->bytes_read_total += bytes_read;
    
    if (bytes_read == 0 || player->bytes_read_total >= player->file_size) {
        player->eof = 1;
    }
    
    return bytes_read;
}

/* Public functions ----------------------------------------------------------*/

/**
 * @brief  MP3 dosyasını aç ve player'ı başlat
 */
int mp3_file_player_open(mp3_file_player_t *player, const char *filename)
{
    FRESULT fres;
    
    if (!player || !filename) {
        return MP3_PLAYER_ERROR;
    }
    
    // Yapıyı sıfırla
    memset(player, 0, sizeof(mp3_file_player_t));
    
    // MP3 dosyasını aç
    fres = f_open(&player->file, filename, FA_READ);
    if (fres != FR_OK) {
        return MP3_PLAYER_FILE_ERROR;
    }
    
    // Dosya boyutunu al
    player->file_size = f_size(&player->file);
    
    // Decoder'ı başlat
    mp3dec_init(&player->mp3dec);
    
    player->mp3_bytes_in_buffer = 0;
    player->mp3_buffer_position = 0;
    player->bytes_read_total = 0;
    player->eof = 0;
    player->loop_enabled = 0;
    
    // İlk veri chunk'ını oku
    if (mp3_file_read_more(player) < 0) {
        f_close(&player->file);
        return MP3_PLAYER_FILE_ERROR;
    }
    
    return MP3_PLAYER_OK;
}

/**
 * @brief  Bir audio frame decode et
 */
int mp3_file_player_decode_frame(mp3_file_player_t *player, int16_t **pcm_out, int *samples)
{
    mp3dec_frame_info_t info;
    int samples_decoded;
    
    if (!player || !pcm_out || !samples) {
        return MP3_PLAYER_ERROR;
    }
    
    // Yeterli veri yoksa daha fazla oku
    if (player->mp3_bytes_in_buffer - player->mp3_buffer_position < 4096) {
        int result = mp3_file_read_more(player);
        if (result < 0) {
            return MP3_PLAYER_FILE_ERROR;
        }
    }
    
    // Eğer veri kalmadıysa ve loop enabled ise başa sar
    if (player->eof && 
        player->mp3_buffer_position >= player->mp3_bytes_in_buffer) {
        
        if (player->loop_enabled) {
            if (mp3_file_player_rewind(player) != MP3_PLAYER_OK) {
                return MP3_PLAYER_ERROR;
            }
        } else {
            return MP3_PLAYER_END_OF_FILE;
        }
    }
    
    // Frame decode et
    samples_decoded = mp3dec_decode_frame(
        &player->mp3dec,
        player->mp3_buffer + player->mp3_buffer_position,
        player->mp3_bytes_in_buffer - player->mp3_buffer_position,
        player->pcm_buffer,
        &info
    );
    
    // Decode başarısız ise
    if (samples_decoded == 0) {
        if (player->eof) {
            if (player->loop_enabled) {
                // Loop için başa sar
                if (mp3_file_player_rewind(player) == MP3_PLAYER_OK) {
                    // Tekrar dene
                    return mp3_file_player_decode_frame(player, pcm_out, samples);
                }
            }
            return MP3_PLAYER_END_OF_FILE;
        }
        return MP3_PLAYER_DECODE_ERROR;
    }
    
    // Buffer pozisyonunu güncelle
    player->mp3_buffer_position += info.frame_bytes;
    
    // İlk frame'de format bilgilerini sakla
    if (player->sample_rate == 0) {
        player->sample_rate = info.hz;
        player->channels = info.channels;
    }
    
    *pcm_out = player->pcm_buffer;
    *samples = samples_decoded;
    
    return samples_decoded;
}

/**
 * @brief  Dosyayı başa sar
 */
int mp3_file_player_rewind(mp3_file_player_t *player)
{
    FRESULT fres;
    
    if (!player) {
        return MP3_PLAYER_ERROR;
    }
    
    // Dosya pozisyonunu başa al
    fres = f_lseek(&player->file, 0);
    if (fres != FR_OK) {
        return MP3_PLAYER_FILE_ERROR;
    }
    
    // Decoder'ı yeniden başlat
    mp3dec_init(&player->mp3dec);
    
    // Buffer'ları sıfırla
    player->mp3_bytes_in_buffer = 0;
    player->mp3_buffer_position = 0;
    player->bytes_read_total = 0;
    player->eof = 0;
    
    // İlk chunk'ı oku
    if (mp3_file_read_more(player) < 0) {
        return MP3_PLAYER_FILE_ERROR;
    }
    
    return MP3_PLAYER_OK;
}

/**
 * @brief  Loop playback enable/disable
 */
void mp3_file_player_set_loop(mp3_file_player_t *player, uint8_t enable)
{
    if (player) {
        player->loop_enabled = enable ? 1 : 0;
    }
}

/**
 * @brief  Dosyayı kapat
 */
void mp3_file_player_close(mp3_file_player_t *player)
{
    if (player) {
        f_close(&player->file);
        memset(player, 0, sizeof(mp3_file_player_t));
    }
}

/**
 * @brief  Player bilgilerini al
 */
void mp3_file_player_get_info(mp3_file_player_t *player, 
                              uint32_t *sample_rate, 
                              uint8_t *channels,
                              uint32_t *file_size)
{
    if (player) {
        if (sample_rate) {
            *sample_rate = player->sample_rate;
        }
        if (channels) {
            *channels = player->channels;
        }
        if (file_size) {
            *file_size = player->file_size;
        }
    }
}

