#ifndef LFS_USER_H
#define LFS_USER_H

#include "lfs.h"
#include <string.h>
#include <stdint.h>

#define LFS_BASE_ADDR      (0x77B00000UL)   // QSPI memory-mapped address (your partition base)
#define LFS_SIZE_BYTES     (5UL * 1024UL * 1024UL)  // 5 MiB
#define LFS_BLOCK_SIZE     (4096U)          // 4KB sector erase
#define LFS_PROG_SIZE      (256U)           // 256B page program
#define LFS_READ_SIZE      (16U)            // can be 16/32
#define LFS_CACHE_SIZE     (256U)           // usually = prog_size
#define LFS_LOOKAHEAD_SIZE (32U)            // 16 or 32 is fine

#define LFS_BLOCK_COUNT    (LFS_SIZE_BYTES / LFS_BLOCK_SIZE) // 1280

int littlefs_mount_ro(void);
void littlefs_list_music(void);
void littlefs_dump_mp3_header(const char *path);
void lfs_list_dir(const char *path);

#endif