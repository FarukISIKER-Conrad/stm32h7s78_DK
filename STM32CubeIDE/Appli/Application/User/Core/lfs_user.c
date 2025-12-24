#include "lfs.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define LFS_BASE_ADDR      (0x77B00000UL)       // QSPI memory-mapped partition base
#define LFS_SIZE_BYTES     (5UL * 1024UL * 1024UL)
#define LFS_BLOCK_SIZE     (4096U)
#define LFS_PROG_SIZE      (256U)
#define LFS_READ_SIZE      (16U)
#define LFS_CACHE_SIZE     (256U)
#define LFS_LOOKAHEAD_SIZE (32U)
#define LFS_BLOCK_COUNT    (LFS_SIZE_BYTES / LFS_BLOCK_SIZE)  // 1280
#define LFS_ERR_ROFS		-1

static lfs_t g_lfs;

// --- Memory-mapped read helper ---
static inline const uint8_t* lfs_mm_ptr(lfs_block_t block, lfs_off_t off) {
    uint32_t addr = (uint32_t)(LFS_BASE_ADDR + (uint32_t)block * LFS_BLOCK_SIZE + (uint32_t)off);
    return (const uint8_t*)addr;
}

static int bd_read(const struct lfs_config *c,
                   lfs_block_t block, lfs_off_t off,
                   void *buffer, lfs_size_t size) {
    (void)c;
    memcpy(buffer, lfs_mm_ptr(block, off), size);
    return 0;
}

// Read-only: deny writes/erase
static int bd_prog(const struct lfs_config *c,
                   lfs_block_t block, lfs_off_t off,
                   const void *buffer, lfs_size_t size) {
    (void)c; (void)block; (void)off; (void)buffer; (void)size;
    return LFS_ERR_ROFS;  // read-only filesystem
}

static int bd_erase(const struct lfs_config *c, lfs_block_t block) {
    (void)c; (void)block;
    return LFS_ERR_ROFS;
}

static int bd_sync(const struct lfs_config *c) {
    (void)c;
    return 0;
}

static const struct lfs_config g_lfs_cfg = {
    .context        = NULL,
    .read           = bd_read,
    .prog           = bd_prog,
    .erase          = bd_erase,
    .sync           = bd_sync,

    .read_size      = LFS_READ_SIZE,
    .prog_size      = LFS_PROG_SIZE,
    .block_size     = LFS_BLOCK_SIZE,
    .block_count    = LFS_BLOCK_COUNT,
    .cache_size     = LFS_CACHE_SIZE,
    .lookahead_size = LFS_LOOKAHEAD_SIZE,
    .block_cycles   = 500,
};

// ---- Call this in APP after memory-mapped mode is enabled ----
int littlefs_mount_ro(void) {
    int err = lfs_mount(&g_lfs, &g_lfs_cfg);

    // IMPORTANT: In this RO setup, do NOT call lfs_format on failure,
    // otherwise you'd need prog/erase and you'd also destroy your prebuilt image.
    return err; // 0 = OK, negative = error
}



void littlefs_list_music(void) {
    lfs_dir_t dir;
    struct lfs_info info;

    if (lfs_dir_open(&g_lfs, &dir, "/music") < 0) {
        printf("dir open failed\r\n");
        return;
    }

    while (true) {
        int res = lfs_dir_read(&g_lfs, &dir, &info);
        if (res <= 0) break;

        printf("%s  type=%d  size=%lu\r\n", info.name, info.type, (unsigned long)info.size);
    }
    lfs_dir_close(&g_lfs, &dir);
}

void littlefs_dump_mp3_header(const char *path) {
    lfs_file_t f;
    uint8_t hdr[16];

    if (lfs_file_open(&g_lfs, &f, path, LFS_O_RDONLY) < 0) {
        printf("file open failed: %s\r\n", path);
        return;
    }

    int n = lfs_file_read(&g_lfs, &f, hdr, sizeof(hdr));
    lfs_file_close(&g_lfs, &f);

    printf("read=%d hdr:", n);
    for (int i=0; i<16 && i<n; i++) printf(" %02X", hdr[i]);
    printf("\r\n");
}

void lfs_list_dir(const char *path)
{
    lfs_dir_t dir;
    struct lfs_info info;

    int err = lfs_dir_open(&g_lfs, &dir, path);
    if (err < 0) {
        printf("lfs_dir_open(%s) failed: %d\r\n", path, err);
        return;
    }

    printf("Listing: %s\r\n", path);

    while (1) {
        int res = lfs_dir_read(&g_lfs, &dir, &info);
        if (res < 0) {
            printf("lfs_dir_read err: %d\r\n", res);
            break;
        }
        if (res == 0) break; // end

        // "." and ".." gelebilir, istersen filtrele
        if (!strcmp(info.name, ".") || !strcmp(info.name, "..")) continue;

        const char *type =
            (info.type == LFS_TYPE_DIR)  ? "DIR " :
            (info.type == LFS_TYPE_REG)  ? "FILE" : "OTHR";

        printf("  %s  %-4s  %lu\r\n", info.name, type, (unsigned long)info.size);
    }

    lfs_dir_close(&g_lfs, &dir);
}

// Get file size
int lfs_get_file_size(const char *path, size_t *size) {
    struct lfs_info info;
    int err = lfs_stat(&g_lfs, path, &info);
    if (err < 0) {
        printf("lfs_stat(%s) failed: %d\r\n", path, err);
        return err;
    }
    
    if (info.type != LFS_TYPE_REG) {
        printf("%s is not a regular file\r\n", path);
        return -1;
    }
    
    *size = info.size;
    printf("File %s size: %lu bytes\r\n", path, (unsigned long)info.size);
    return 0;
}

// Read entire file into buffer
int lfs_read_file(const char *path, uint8_t *buffer, size_t buffer_size, size_t *bytes_read) {
    lfs_file_t file;
    
    // Open file
    int err = lfs_file_open(&g_lfs, &file, path, LFS_O_RDONLY);
    if (err < 0) {
        printf("lfs_file_open(%s) failed: %d\r\n", path, err);
        return err;
    }
    
    // Get file size
    lfs_soff_t file_size = lfs_file_size(&g_lfs, &file);
    if (file_size < 0) {
        printf("lfs_file_size failed: %d\r\n", (int)file_size);
        lfs_file_close(&g_lfs, &file);
        return (int)file_size;
    }
    
    // Check buffer size
    if ((size_t)file_size > buffer_size) {
        printf("Buffer too small. File: %lu, Buffer: %lu\r\n", 
               (unsigned long)file_size, (unsigned long)buffer_size);
        lfs_file_close(&g_lfs, &file);
        return -1;
    }
    
    // Read file
    lfs_ssize_t read_result = lfs_file_read(&g_lfs, &file, buffer, (size_t)file_size);
    if (read_result < 0) {
        printf("lfs_file_read failed: %d\r\n", (int)read_result);
        lfs_file_close(&g_lfs, &file);
        return (int)read_result;
    }
    
    *bytes_read = (size_t)read_result;
    printf("Successfully read %lu bytes from %s\r\n", (unsigned long)*bytes_read, path);
    
    // Close file
    lfs_file_close(&g_lfs, &file);
    return 0;
}
