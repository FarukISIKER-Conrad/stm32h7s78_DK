/*
 * minimp3.c - Minimalistic MP3 decoder implementation
 * 
 * This file includes the minimp3 header-only library with implementation.
 * The minimp3.h header contains the full MP3 decoder implementation.
 * 
 * For more information: https://github.com/lieff/minimp3
 */

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_SIMD  /* Disable SIMD for ARM compatibility */

#include "minimp3.h"

/*
 * No additional code needed here.
 * All functions (mp3dec_init, mp3dec_decode_frame, etc.) are 
 * implemented in minimp3.h when MINIMP3_IMPLEMENTATION is defined.
 */
 