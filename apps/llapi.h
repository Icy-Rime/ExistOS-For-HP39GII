#pragma once

#include <stdint.h>
#include <stdbool.h>


#define HCATTR   __attribute__((naked)) __attribute__((target("arm")))
#define DECDEF_LLAPI_SWI(ret, name, pars, SWINum)   \
    ret HCATTR name pars;


#define LL_SWI_BASE (0xEE00)
#define LLAPI_SWI_BASE                     (0xD700)

#define LLAPI_APP_DELAY_MS                 (LLAPI_SWI_BASE + 1)
#define LLAPI_APP_STDOUT_PUTC              (LLAPI_SWI_BASE + 2)
#define LLAPI_APP_GET_RAM_SIZE             (LLAPI_SWI_BASE + 3)
#define LLAPI_APP_GET_TICK_MS              (LLAPI_SWI_BASE + 4)
#define LLAPI_APP_GET_TICK_US              (LLAPI_SWI_BASE + 5)
#define LLAPI_APP_DISP_PUT_P               (LLAPI_SWI_BASE + 6)
#define LLAPI_APP_DISP_PUT_HLINE           (LLAPI_SWI_BASE + 7)
#define LLAPI_APP_DISP_GET_P               (LLAPI_SWI_BASE + 8)
#define LLAPI_APP_QUERY_KEY                (LLAPI_SWI_BASE + 9)
#define LLAPI_APP_RTC_GET_S                (LLAPI_SWI_BASE + 10)
#define LLAPI_APP_RTC_SET_S                (LLAPI_SWI_BASE + 11) 
#define LLAPI_APP_IS_KEY_DOWN              (LLAPI_SWI_BASE + 13)
#define LLAPI_APP_DISP_PUT_KSTRING         (LLAPI_SWI_BASE + 14)
#define LLAPI_APP_DISP_CLEAN               (LLAPI_SWI_BASE + 15)
#define LLAPI_APP_DISP_PUT_HLINE_LEN       (LLAPI_SWI_BASE + 16)

#define LLAPI_MMAP             (LLAPI_SWI_BASE + 20)
#define LLAPI_MUNMAP           (LLAPI_SWI_BASE + 21)


#define LLAPI_THREAD_CREATE                   (LLAPI_SWI_BASE + 160)
#define LLAPI_APP_EXIT                        (LLAPI_SWI_BASE + 161)
#define LLAPI_SET_PERF_LEVEL                  (LLAPI_SWI_BASE + 170)




#define LL_SWI_ICACHE_INV               (LL_SWI_BASE + 117)
#define LL_SWI_DCACHE_CLEAN             (LL_SWI_BASE + 118)

#define LL_SWI_FS_SIZE                 (LL_SWI_BASE + 119)
#define LL_SWI_FS_REMOVE               (LL_SWI_BASE + 120)
#define LL_SWI_FS_RENAME               (LL_SWI_BASE + 121)
#define LL_SWI_FS_STAT                 (LL_SWI_BASE + 122)
#define LL_SWI_FS_OPEN                 (LL_SWI_BASE + 123)
#define LL_SWI_FS_CLOSE                (LL_SWI_BASE + 124)
#define LL_SWI_FS_SYNC                 (LL_SWI_BASE + 125)
#define LL_SWI_FS_READ                 (LL_SWI_BASE + 126)
#define LL_SWI_FS_WRITE                (LL_SWI_BASE + 127)
#define LL_SWI_FS_SEEK                 (LL_SWI_BASE + 128)
#define LL_SWI_FS_REWIND               (LL_SWI_BASE + 129)
#define LL_SWI_FS_TRUNCATE             (LL_SWI_BASE + 130)
#define LL_SWI_FS_TELL                 (LL_SWI_BASE + 131)

#define LL_SWI_FS_DIR_MKDIR               (LL_SWI_BASE + 132)
#define LL_SWI_FS_DIR_OPEN                (LL_SWI_BASE + 133)
#define LL_SWI_FS_DIR_CLOSE               (LL_SWI_BASE + 134)
#define LL_SWI_FS_DIR_SEEK                (LL_SWI_BASE + 135)
#define LL_SWI_FS_DIR_TELL                (LL_SWI_BASE + 136)
#define LL_SWI_FS_DIR_REWIND              (LL_SWI_BASE + 137)
#define LL_SWI_FS_DIR_READ                (LL_SWI_BASE + 138)
#define LL_SWI_FS_DIR_GET_CUR_TYPE                (LL_SWI_BASE + 139)
#define LL_SWI_FS_DIR_GET_CUR_NAME                (LL_SWI_BASE + 140)
#define LL_SWI_FS_DIR_GET_CUR_SIZE                (LL_SWI_BASE + 141)

#define LL_SWI_FS_GET_FOBJ_SZ                  (LL_SWI_BASE + 142)
#define LL_SWI_FS_GET_DIROBJ_SZ                (LL_SWI_BASE + 143)

#define FS_FILE_TYPE_REG   (1)
#define FS_FILE_TYPE_DIR   (2)

#define FS_O_RDONLY    0         /* +1 == FREAD */
#define FS_O_WRONLY    1         /* +1 == FWRITE */
#define FS_O_RDWR      2         /* +1 == FREAD|FWRITE */
#define FS_O_CREAT     0x0200    /* open with file create */
#define FS_O_EXCL      0x0800    /* error on open if file exists */
#define FS_O_TRUNC     0x0400    /* open with truncation */
#define FS_O_APPEND    0x0008    /* append (writes guaranteed at the end) */

#define FS_SEEK_SET   0          /* Seek relative to an absolute position */
#define FS_SEEK_CUR   1          /* Seek relative to the current file position */
#define FS_SEEK_END   2          /* Seek relative to the end of the file */

#define FS_ERR_OK           0,    /* No error */
#define FS_ERR_IO           -5,   /* Error during device operation */
#define FS_ERR_CORRUPT      -84,  /* Corrupted */
#define FS_ERR_NOENT        -2,   /* No directory entry */
#define FS_ERR_EXIST        -17,  /* Entry already exists */
#define FS_ERR_NOTDIR       -20,  /* Entry is not a dir */
#define FS_ERR_ISDIR        -21,  /* Entry is a dir */
#define FS_ERR_NOTEMPTY     -39,  /* Dir is not empty */
#define FS_ERR_BADF         -9,   /* Bad file number */
#define FS_ERR_FBIG         -27,  /* File too large */
#define FS_ERR_INVAL        -22,  /* Invalid parameter */
#define FS_ERR_NOSPC        -28,  /* No space left on device */
#define FS_ERR_NOMEM        -12,  /* No more memory available */
#define FS_ERR_NOATTR       -61,  /* No data/attr available */
#define FS_ERR_NAMETOOLONG  -36,  /* File name too long */

typedef void* fs_obj_t;
typedef void* fs_dir_obj_t;

#ifndef MMAP_INFO
#define MMAP_INFO
typedef struct mmap_info
{
    uint32_t map_to;
    const char *path;
    uint32_t offset;
    uint32_t size;
    bool writable;
    bool writeback;
}mmap_info;
#endif

#ifdef __cplusplus
    extern "C" {
#endif


DECDEF_LLAPI_SWI(void,          llapi_delay_ms,              (uint32_t ms),                                     LLAPI_APP_DELAY_MS)
DECDEF_LLAPI_SWI(void,          llapi_putc,                  (char c),                                          LLAPI_APP_STDOUT_PUTC)
DECDEF_LLAPI_SWI(uint32_t,      llapi_get_ram_size,          (void),                                            LLAPI_APP_GET_RAM_SIZE)
DECDEF_LLAPI_SWI(uint32_t,      llapi_get_tick_ms,           (void),                                            LLAPI_APP_GET_TICK_MS)
DECDEF_LLAPI_SWI(uint32_t,      llapi_get_tick_us,           (void),                                            LLAPI_APP_GET_TICK_US)
DECDEF_LLAPI_SWI(int,           llapi_query_key,             (void),                                            LLAPI_APP_QUERY_KEY)
DECDEF_LLAPI_SWI(int,           llapi_is_key_down,           (uint32_t key),                                    LLAPI_APP_IS_KEY_DOWN)
DECDEF_LLAPI_SWI(uint32_t,      llapi_rtc_get_s,             (void),                                            LLAPI_APP_RTC_GET_S)
DECDEF_LLAPI_SWI(uint32_t,      llapi_rtc_set_s,             (uint32_t s),                                      LLAPI_APP_RTC_SET_S)
          
DECDEF_LLAPI_SWI(void,          llapi_disp_put_point,        (uint32_t x, uint32_t y, uint32_t c),              LLAPI_APP_DISP_PUT_P)
DECDEF_LLAPI_SWI(uint32_t,      llapi_disp_get_point,        (uint32_t x, uint32_t y),                          LLAPI_APP_DISP_GET_P)
DECDEF_LLAPI_SWI(void,          llapi_disp_put_hline,        (uint32_t y, char *dat),                           LLAPI_APP_DISP_PUT_HLINE)
DECDEF_LLAPI_SWI(void,          llapi_disp_put_hline_len,    (uint32_t y, char *dat, uint32_t len),             LLAPI_APP_DISP_PUT_HLINE_LEN)
DECDEF_LLAPI_SWI(void,          llapi_disp_put_kstr,         (uint32_t x, uint32_t y, char *s, uint32_t fgbg),  LLAPI_APP_DISP_PUT_KSTRING)
DECDEF_LLAPI_SWI(void,          llapi_disp_clean,            (uint32_t bg),                                     LLAPI_APP_DISP_CLEAN)

DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_get_dirobj_sz,      (void)                                      ,LL_SWI_FS_GET_DIROBJ_SZ      );
DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_get_fobj_sz,        (void)                                      ,LL_SWI_FS_GET_FOBJ_SZ        );
DECDEF_LLAPI_SWI(int,           llapi_fs_size,                  (fs_obj_t fobj)                             ,LL_SWI_FS_SIZE               );
DECDEF_LLAPI_SWI(int,           llapi_fs_remove,             (const char *path)                          ,LL_SWI_FS_REMOVE             );
DECDEF_LLAPI_SWI(int,           llapi_fs_rename,             (const char *oldpath, const char *newpath)  ,LL_SWI_FS_RENAME             );
DECDEF_LLAPI_SWI(int,           llapi_fs_open,               (fs_obj_t fobj, const char *path, int flag) ,LL_SWI_FS_OPEN               );
DECDEF_LLAPI_SWI(int,           llapi_fs_close,              (fs_obj_t fobj)                             ,LL_SWI_FS_CLOSE              );
DECDEF_LLAPI_SWI(int,           llapi_fs_sync,               (fs_obj_t fobj)                             ,LL_SWI_FS_SYNC               );
DECDEF_LLAPI_SWI(int,           llapi_fs_read,               (fs_obj_t fobj, void* buf, uint32_t size)   ,LL_SWI_FS_READ               );
DECDEF_LLAPI_SWI(int,           llapi_fs_write,              (fs_obj_t fobj, void* buf, uint32_t size)   ,LL_SWI_FS_WRITE              );
DECDEF_LLAPI_SWI(int,           llapi_fs_seek,               (fs_obj_t fobj, uint32_t off, int whence)   ,LL_SWI_FS_SEEK               );
DECDEF_LLAPI_SWI(int,           llapi_fs_rewind,             (fs_obj_t fobj)                             ,LL_SWI_FS_REWIND             );
DECDEF_LLAPI_SWI(int,           llapi_fs_truncate,           (fs_obj_t fobj, uint32_t size)              ,LL_SWI_FS_TRUNCATE           );
DECDEF_LLAPI_SWI(int,           llapi_fs_tell,               (fs_obj_t fobj)                             ,LL_SWI_FS_TELL               );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_mkdir,          (const char* path)                          ,LL_SWI_FS_DIR_MKDIR          );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_open,           (fs_dir_obj_t dir_obj, const char* path)    ,LL_SWI_FS_DIR_OPEN           );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_close,          (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_CLOSE          );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_seek,           (fs_dir_obj_t dir_obj, uint32_t off)        ,LL_SWI_FS_DIR_SEEK           );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_tell,           (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_TELL           );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_rewind,         (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_REWIND         );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_read,           (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_READ           );
DECDEF_LLAPI_SWI(const char *,  llapi_fs_dir_cur_item_name,  (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_GET_CUR_NAME   );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_cur_item_size,  (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_GET_CUR_SIZE   );
DECDEF_LLAPI_SWI(int,           llapi_fs_dir_cur_item_type,  (fs_dir_obj_t dir_obj)                      ,LL_SWI_FS_DIR_GET_CUR_TYPE   );


DECDEF_LLAPI_SWI(int,               llapi_mmap,              (mmap_info *info)                           ,LLAPI_MMAP                  );
DECDEF_LLAPI_SWI(void,              llapi_mumap,             (int map)                                   ,LLAPI_MUNMAP                );


DECDEF_LLAPI_SWI(int,           llapi_thread_create,     (void *code, uint32_t *stack, uint32_t stackSz, void *par)  ,LLAPI_THREAD_CREATE     );
DECDEF_LLAPI_SWI(void,          llapi_app_stop,          (void)                                                      ,LLAPI_APP_EXIT          );
DECDEF_LLAPI_SWI(void,          llapi_set_perf_level,    (int level)                                                 ,LLAPI_SET_PERF_LEVEL    );

DECDEF_LLAPI_SWI(void,          llapi_invalidate_icache,    (void)                                                   ,LL_SWI_ICACHE_INV       );
DECDEF_LLAPI_SWI(void,          llapi_clean_dcache,         (void *base, uint32_t size)                              ,LL_SWI_DCACHE_CLEAN       );


#ifdef __cplusplus          
    }          
#endif


