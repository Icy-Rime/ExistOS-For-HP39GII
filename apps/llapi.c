#include <stdint.h>
#include "llapi.h"

#undef DECDEF_LLAPI_SWI
#define DECDEF_LLAPI_SWI(ret, name, pars, SWINum)                       \
    ret HCATTR name pars                                                 \
    {                                                                   \
        __asm volatile("push {lr}");                                    \
        __asm volatile("swi %0" :: "i"(SWINum));                        \
        __asm volatile("pop {lr}");                                    \
        __asm volatile("bx lr");                                        \
    }

DECDEF_LLAPI_SWI(void,          llapi_delay_ms,              (uint32_t ms),          LLAPI_APP_DELAY_MS)
DECDEF_LLAPI_SWI(void,          llapi_putc,                  (char c),               LLAPI_APP_STDOUT_PUTC)
DECDEF_LLAPI_SWI(uint32_t,      llapi_get_ram_size,          (void),                 LLAPI_APP_GET_RAM_SIZE)
    
DECDEF_LLAPI_SWI(uint32_t,      llapi_get_tick_ms,           (void),                 LLAPI_APP_GET_TICK_MS)
DECDEF_LLAPI_SWI(uint32_t,      llapi_get_tick_us,           (void),                 LLAPI_APP_GET_TICK_US)
DECDEF_LLAPI_SWI(int,           llapi_query_key,             (void),                 LLAPI_APP_QUERY_KEY)
DECDEF_LLAPI_SWI(int,           llapi_is_key_down,           (uint32_t key),         LLAPI_APP_IS_KEY_DOWN)
DECDEF_LLAPI_SWI(uint32_t,      llapi_rtc_get_s,             (void),                                      LLAPI_APP_RTC_GET_S)
DECDEF_LLAPI_SWI(uint32_t,      llapi_rtc_set_s,             (uint32_t s),                                LLAPI_APP_RTC_SET_S)
    
    
DECDEF_LLAPI_SWI(void,          llapi_disp_put_point,        (uint32_t x, uint32_t y, uint32_t c),    LLAPI_APP_DISP_PUT_P)
DECDEF_LLAPI_SWI(uint32_t,      llapi_disp_get_point,        (uint32_t x, uint32_t y),           LLAPI_APP_DISP_GET_P)
DECDEF_LLAPI_SWI(void,          llapi_disp_put_hline,        (uint32_t y, char *dat),            LLAPI_APP_DISP_PUT_HLINE)
DECDEF_LLAPI_SWI(void,          llapi_disp_put_hline_len,    (uint32_t y, char *dat, uint32_t len),             LLAPI_APP_DISP_PUT_HLINE_LEN)
DECDEF_LLAPI_SWI(void,          llapi_disp_put_kstr,         (uint32_t x, uint32_t y, char *s, uint32_t fgbg),  LLAPI_APP_DISP_PUT_KSTRING)
DECDEF_LLAPI_SWI(void,          llapi_disp_clean,            (uint32_t bg),                                     LLAPI_APP_DISP_CLEAN)

DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_get_dirobj_sz,      (void)                                      ,LL_SWI_FS_GET_DIROBJ_SZ      );
DECDEF_LLAPI_SWI(uint32_t,      llapi_fs_get_fobj_sz,        (void)                                      ,LL_SWI_FS_GET_FOBJ_SZ        );
DECDEF_LLAPI_SWI(int,           llapi_fs_size,               (fs_obj_t fobj)                             ,LL_SWI_FS_SIZE               );
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


DECDEF_LLAPI_SWI(int,           llapi_thread_create,     (void *code, uint32_t *stack, uint32_t stackSz, void *par)  ,LLAPI_THREAD_CREATE    );
DECDEF_LLAPI_SWI(void,          llapi_set_perf_level,    (int level)                                                 ,LLAPI_SET_PERF_LEVEL    );


DECDEF_LLAPI_SWI(void,          llapi_app_stop,          (void)                                                      ,LLAPI_APP_EXIT          );
DECDEF_LLAPI_SWI(void,          llapi_invalidate_icache,    (void)                                                   ,LL_SWI_ICACHE_INV       );
DECDEF_LLAPI_SWI(void,          llapi_clean_dcache,         (void *base, uint32_t size)                              ,LL_SWI_DCACHE_CLEAN       );
