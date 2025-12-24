/**
  ******************************************************************************
  * @file    mp3_file_player.h
  * @brief   MP3 File Player with FatFS and minimp3
  *          QSPI Flash üzerinden MP3 dosyalarını okuyup decode eder
  ******************************************************************************
  */

#ifndef __MP3_FILE_PLAYER_H
#define __MP3_FILE_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "ff.h"
#include "minimp3.h"

/* Exported types ------------------------------------------------------------*/

/* Return codes */
#define MP3_PLAYER_OK               0
#define MP3_PLAYER_ERROR           -1
#define MP3_PLAYER_FILE_ERROR      -2
#define MP3_PLAYER_DECODE_ERROR    -3
#define MP3_PLAYER_END_OF_FILE     -4
#define MP3_PLAYER_NO_MEMORY       -5

/* Configuration */
#define MP3_READ_BUFFER_SIZE       (8*1024)     // 8KB okuma buffer
#define MP3_PCM_BUFFER_SIZE        (1152*4*2)   // Decode buffer (stereo, max frame size)

/**
 * @brief MP3 File Player Handle
 */
typedef struct {
    FIL file;                                    /* FatFS file handle */
    mp3dec_t mp3dec;                            /* minimp3 decoder */
    
    uint8_t mp3_buffer[MP3_READ_BUFFER_SIZE];   /* MP3 veri okuma buffer */
    int16_t pcm_buffer[MP3_PCM_BUFFER_SIZE];    /* Decoded PCM buffer */
    
    size_t mp3_bytes_in_buffer;                 /* Buffer'da mevcut byte sayısı */
    size_t mp3_buffer_position;                 /* Buffer içinde okuma pozisyonu */
    
    uint32_t file_size;                         /* Toplam dosya boyutu */
    uint32_t bytes_read_total;                  /* Toplam okunan byte */
    
    uint8_t eof;                                /* End of file flag */
    uint8_t loop_enabled;                       /* Loop playback */
    
    uint32_t sample_rate;                       /* Decoded sample rate */
    uint8_t channels;                           /* Decoded channels */
} mp3_file_player_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  MP3 dosyasını aç ve player'ı başlat
 * @param  player: Player handle pointer
 * @param  filename: MP3 dosya yolu (örn: "0:/music/guitar.mp3")
 * @retval MP3_PLAYER_OK on success, error code otherwise
 */
int mp3_file_player_open(mp3_file_player_t *player, const char *filename);

/**
 * @brief  Bir audio frame decode et
 * @param  player: Player handle pointer
 * @param  pcm_out: Decoded PCM data pointer (output)
 * @param  samples: Decoded sample sayısı (mono/stereo için channel başına)
 * @retval >0: samples decoded, 0: EOF, <0: error
 */
int mp3_file_player_decode_frame(mp3_file_player_t *player, int16_t **pcm_out, int *samples);

/**
 * @brief  Dosyayı başa sar (loop için)
 * @param  player: Player handle pointer
 * @retval MP3_PLAYER_OK on success
 */
int mp3_file_player_rewind(mp3_file_player_t *player);

/**
 * @brief  Loop playback enable/disable
 * @param  player: Player handle pointer
 * @param  enable: 1=enable, 0=disable
 */
void mp3_file_player_set_loop(mp3_file_player_t *player, uint8_t enable);

/**
 * @brief  Dosyayı kapat ve kaynakları serbest bırak
 * @param  player: Player handle pointer
 */
void mp3_file_player_close(mp3_file_player_t *player);

/**
 * @brief  Player bilgilerini al
 * @param  player: Player handle pointer
 * @param  sample_rate: Output sample rate pointer (can be NULL)
 * @param  channels: Output channels pointer (can be NULL)
 * @param  file_size: Output file size pointer (can be NULL)
 */
void mp3_file_player_get_info(mp3_file_player_t *player, 
                              uint32_t *sample_rate, 
                              uint8_t *channels,
                              uint32_t *file_size);

#ifdef __cplusplus
}
#endif

#endif /* __MP3_FILE_PLAYER_H */

