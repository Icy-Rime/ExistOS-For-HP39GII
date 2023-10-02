#include <fcntl.h>

#include "app_runtime.h"
#include "queue.h"
#include "../apps/llapi.h"


#define STATE_SAVE_DIR      "/expsave"

typedef struct file_list_t
{
    struct file_list_t *next;
    char *fname;
    int type;
    uint32_t size;
}file_list_t;

file_list_t *app_sel_flist = NULL;



#define EXP_MAX_ALLOW_THREADS   (5)
#define EXP_MAX_ALLOW_MMAPS     (5)

QueueHandle_t app_api_queue;

static void *exp_mem_wrap_alloc = NULL;
static uint32_t exp_mem_wrap_at = 0;
static uint32_t exp_mem_size = 0;

//static volatile bool app_running = false;
static TaskHandle_t app_selector_handle;

static StaticTask_t app_tcb[EXP_MAX_ALLOW_THREADS];
static TaskHandle_t app_task_handle[EXP_MAX_ALLOW_THREADS];

static int exp_mmap_handle[EXP_MAX_ALLOW_MMAPS];
static mmap_info exp_mmap_info[EXP_MAX_ALLOW_MMAPS];

static int exp_self_mmap = 0;

static uint8_t tid = 0; 

uint32_t exp_main_thread_stack_sz = 0; 

void pre_save_tcb(TaskHandle_t dest);
void tcb_restore(TaskHandle_t ref, TaskHandle_t dest);

//static uint8_t app_ram[380 * 1024] __aligned(1024);
extern uint8_t app_ram;
extern uint8_t app_ram_end;
static void clean_flist(file_list_t **flist)
{
    if(*flist)
    {
        file_list_t *c = *flist;
        file_list_t *last = *flist;
        while (c)
        {
            free(c->fname);
            last = c;
            c = c->next;
            free(last);
        } 
        *flist = NULL;
    }
}

void app_selector_start()
{
    if(!app_selector_handle)
    { 
        void app_selector(void *__);
        xTaskCreate(app_selector, "app_selector", 440, NULL, configMAX_PRIORITIES - 4, &app_selector_handle);
    }
}

void app_selector_stop()
{
    if(app_selector_handle)
    {
        vTaskDelete(app_selector_handle);
        app_selector_handle = NULL;
        clean_flist(&app_sel_flist);
    }
}

bool app_is_running()
{
    for(int i = 0; i < EXP_MAX_ALLOW_THREADS; i++)
    {
        if(app_task_handle[i])
            return true;
    }
    return false;
}



uint32_t app_get_ram_size()
{
    return exp_mem_size;
}

uint32_t runPar;
static void exp_thread_start(void *par) {

    __asm volatile("mrs r1,cpsr_all");
    __asm volatile("bic r1,r1,#0x1f");
    __asm volatile("orr r1,r1,#0x10");
    __asm volatile("msr cpsr_all,r1"); 
    

    void (*r)();
    r = par;
    r(runPar);
    //__asm volatile("mov r1,r0");
    //__asm volatile("ldr r0,=runPar");
    //__asm volatile("ldr r0,[r0]");
    //__asm volatile("blx r1");

    vTaskDelete(NULL);
    while (1) { 
        vTaskDelay(pdMS_TO_TICKS(1000));
        ;
    }
}


mmap_info expfile_mmap;

void app_stop()
{
    
    if(!app_is_running())
        return;
    if(!app_task_handle)
        return;

    vTaskSuspendAll();

    free((void *)expfile_mmap.path);

    for(int i = 0; i < EXP_MAX_ALLOW_THREADS; i++)
    {
        if(app_task_handle[i])
        {
            vTaskDelete(app_task_handle[i]);
            app_task_handle[i] = NULL;
        }
    } 
 
    ll_set_exp_mem_wrap(0, 0);
    //free(exp_mem_wrap_alloc);
    exp_mem_wrap_alloc = NULL;
 
    ll_mumap(exp_self_mmap);
    printf("trim exp main mem\r\n");
    for(uint32_t i = APP_RAM_MAP_ADDR; i < APP_RAM_MAP_ADDR + exp_mem_size; i+=1024)
    {
        ll_mm_trim_vaddr(i & ~(1023));
    }

    for(int i = 0; i < EXP_MAX_ALLOW_MMAPS; i++)
    {
        if(exp_mmap_handle[i] > 0)
        {
            ll_mumap(exp_mmap_handle[i]);
            exp_mmap_handle[i] = -1;
        }
    }



    xPortHeapTrimFreePage();
    exp_mem_size = 0;

    memset(app_tcb, 0, sizeof(app_tcb));
    memset(app_task_handle, 0, sizeof(app_task_handle));
    memset(exp_mmap_handle, 0, sizeof(exp_mmap_handle));
    memset(exp_mmap_info, 0, sizeof(exp_mmap_info));
    
    bsp_set_perf_level(4);

    xTaskResumeAll();
    vTaskDelay(pdMS_TO_TICKS(50)); 
}
 

void app_start()
{
    if(app_is_running())
    {
        return;
    }
    if(exp_self_mmap < 0)
    {
        return;
    }
    tid = 0;
    char tid_buf[16];
    
    bsp_set_perf_level(0);
    
    bsp_diaplay_clean(0xFF); 

    uint32_t *stackSz = (uint32_t *)(APP_ROM_MAP_ADDR + 8);
 
    if(*stackSz != 0xE1A00000)
    {
        exp_main_thread_stack_sz = *stackSz;
        printf("SET Stack Size:%ld\r\n",  exp_main_thread_stack_sz );
    }

    if(exp_main_thread_stack_sz == 0)
    {
        exp_main_thread_stack_sz = 400;
    }

    vTaskSuspendAll();

    
    //uint32_t free_mem = getFreeMemSz() - 16*1024;
    //exp_mem_wrap_alloc = malloc(free_mem);
    //exp_mem_wrap_at = (uint32_t)exp_mem_wrap_alloc;
    //while((exp_mem_wrap_at % 1024))
    //{
    //    exp_mem_wrap_at++;
    //}  
    //exp_mem_size = (free_mem/1024) - 1;
    //ll_set_exp_mem_wrap(exp_mem_wrap_at, exp_mem_size);
    //exp_mem_size = exp_mem_size * 1024;

    exp_mem_wrap_at = (uint32_t)&app_ram;
    exp_mem_size = (uint32_t)&app_ram_end - exp_mem_wrap_at;
    ll_set_exp_mem_wrap(exp_mem_wrap_at, exp_mem_size/1024);
    exp_mem_size = exp_mem_size;
    
    printf("Allocate App Mem:%ld\r\n", exp_mem_size);
    
    sprintf(tid_buf, "app_t%d", tid);
    app_task_handle[tid] = xTaskCreateStatic(
          exp_thread_start, 
          tid_buf, 
          exp_main_thread_stack_sz, 
          (void *)(APP_ROM_MAP_ADDR + 32)
          , 1,
          (StackType_t *const)(APP_RAM_MAP_ADDR  + exp_mem_size - exp_main_thread_stack_sz * 4 - 4) , 
          &app_tcb[tid]); //APP_RAM_MAP_ADDR  + exp_mem_size - 4 - app_stack_sz * 4
    //xTaskCreate(app_main_thread, "app_t0", 400, NULL, 1, &app_task_handle);
    tid++;

    gdb_attach_to_task(app_task_handle[0]);
    xTaskResumeAll(); 
}


uint32_t app_get_exp_sz(char *path)
{
    fs_obj_t f = malloc(ll_fs_get_fobj_sz());
    int res = ll_fs_open(f, path, O_RDONLY);
    if(res)
    {
        free(f);
        return 0;
    }
    int sz = ll_fs_size(f);
    ll_fs_close(f);
    free(f);
    return sz;
}


void app_pre_start(char *path, bool sideload, uint32_t sideload_sz)
{
    if(app_is_running())
        return;


    app_stop();


    expfile_mmap.map_to = APP_ROM_MAP_ADDR;
    expfile_mmap.offset = 0;
    expfile_mmap.writable = false;
    expfile_mmap.writeback = false;
    //if(sideload)
    //{
    //    mf.size = sideload_sz;
    //    mf.path = calloc(1, 3);
    //    memcpy((void *)mf.path,(void *)"\03",1);
    //}else
    {
        expfile_mmap.path = calloc(1, 64);
        strncpy((char *)expfile_mmap.path, path, 63);
        uint32_t expSz = app_get_exp_sz(path);
        //int32_t remainSz = expSz;
        printf("app rom sz:%d\r\n", expSz);
        expfile_mmap.size = 0;
        //mmaps[0] = ll_mmap(&expfile_mmap);
        exp_self_mmap = ll_mmap(&expfile_mmap);
        printf("A mmap:%d, off:%08lX,sz:%ld,  %s\r\n", exp_self_mmap, expfile_mmap.offset, expfile_mmap.size,  expfile_mmap.path);       
    }

    //mmap = ll_mmap(&expfile_mmap);
    
}
 
static char *get_name_from_path(char *path)
{
    
    uint32_t i = strlen(path) - 1;
    while ((path[i] != '/') && i)
    {
        i--;
    }
    i++;
    return &path[i];
}

int _ON_LongPress = 0;
#include <assert.h>

static int get_save_id(char *expName)
{ 

    fs_dir_obj_t dir;
    int id = 0;
    char *search_name = calloc(1,128);
    dir = calloc(1, ll_fs_get_dirobj_sz());
    if(!dir)
    {
        printf("Calloc ERR\r\n");
    }
    int ret = ll_fs_dir_open(dir, STATE_SAVE_DIR);
    if(ret < 0)
    {
        free(dir);
        free(search_name);
        return -1;
    }
        
search:
    sprintf(search_name, "%s.%d.sav",expName, id);
    while(ll_fs_dir_read(dir) > 0)
    {
        //printf("cmp:%s == %s\r\n", search_name,ll_fs_dir_cur_item_name(dir) );
        if(!strcmp(search_name, ll_fs_dir_cur_item_name(dir)))
        {
            id++;
            ll_fs_dir_rewind(dir);
            goto search;
        }
    }

    ll_fs_dir_close(dir);


    free(dir);
    free(search_name);
    return id;
}

void app_api_task(void *p)
{ 
    app_api_info_t info;
    uint32_t ticks = 0;
    for(;;)
    {
        if(xQueueReceive(app_api_queue, &info, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            switch (info.code)
            {
                case LLAPI_THREAD_CREATE:
                    info.context[0] = -1;
                    for(int i = 0; i < EXP_MAX_ALLOW_THREADS; i++)
                    {
                        if(app_task_handle[i] == 0)
                        {
                            char tbuf[8];
                            sprintf(tbuf, "app_t%d", i);
                            //app_task_handle[i] = xTaskCreateStatic(
                            //    (TaskFunction_t)info.par0, 
                            //    tbuf, 
                            //    info.par2 / 4, 
                            //    (void *)info.par3,
                            //    1, 
                            //    (StackType_t *)info.par1, 
                            //    &app_tcb[i]);
                            runPar = info.par3;
                            app_task_handle[i] = xTaskCreateStatic(
                                (TaskFunction_t)exp_thread_start, 
                                tbuf, 
                                info.par2 / 4, 
                                (void *)info.par0,
                                1, 
                                (StackType_t *)info.par1, 
                                &app_tcb[i]);
                            info.context[0] = i;
                            break;
                        }
                    }
                    break;
                case LLAPI_APP_DELAY_MS:
                    vOtherTaskDelay(info.task, pdMS_TO_TICKS(info.par0));
                    break; 
                
                case LLAPI_MMAP:
                    for(int i = 0; i < EXP_MAX_ALLOW_MMAPS; i++)
                    {
                        if(exp_mmap_handle[i] == 0)
                        {
                            if(ll_vaddr_vaild(info.par0))
                            {
                                int ret = ll_mmap((mmap_info *)info.par0);
                                if(ret > 0)
                                {
                                    memcpy(&exp_mmap_info[i], (mmap_info *)info.par0, sizeof(mmap_info));
                                    exp_mmap_handle[i] = ret;
                                    break;
                                }else{
                                    printf("mmap error:%d\r\n",ret);
                                    info.context[0] = -1;
                                }
                            }else{
                                printf("mmap error: invalid mf addr %08X\r\n",info.par0);
                                info.context[0] = -1;
                            }
                        }
                    }
                    break;
                case LLAPI_MUNMAP:
                    if(info.par0 < EXP_MAX_ALLOW_MMAPS)
                    {
                        if(exp_mmap_handle[info.par0] > 0)
                            ll_mumap(exp_mmap_handle[info.par0]);
                            exp_mmap_handle[info.par0] = -1;
                            memset(&exp_mmap_info[info.par0], 0, sizeof(mmap_info));
                    }
                    break;

            default:
                break;
            }
        }

        if(bsp_is_key_down(KEY_ON) && bsp_is_key_down(KEY_F6))
        {
            bsp_reset();
        }

        if(bsp_is_key_down(KEY_ON) && bsp_is_key_down(KEY_F5))
        {
            if(app_is_running())
            {
                printf("Save state\r\n");
                ll_fs_dir_mkdir(STATE_SAVE_DIR);
                _ON_LongPress = 0; 

                vTaskSuspendAll();
                char *exp_name = get_name_from_path((char *)expfile_mmap.path);
                int saveid = get_save_id(exp_name);
                char *save_path = calloc(1,128);
                sprintf(save_path, STATE_SAVE_DIR "/%s.%d.sav",exp_name, saveid );
                
                fs_obj_t f = calloc(1, ll_fs_get_fobj_sz());
                assert(f);
                int ret = ll_fs_open(f, save_path , O_CREAT | O_WRONLY | O_TRUNC);
                if(ret < 0)
                {
                    printf("save err:%d\r\n",ret);
                }
                uint8_t wr_u8;
                uint32_t tcb_floc;
                uint32_t mmapinfo_floc;
                uint32_t mmapfpath_floc;
                uint32_t mem_floc;
                uint32_t scr_floc;
                uint32_t mem_bitmap_floc;

                ll_fs_write(f, "EXPSAVE", 7);

                wr_u8 = 0;
                ll_fs_seek(f, 16*4, SEEK_SET);
                ll_fs_write(f, (void *)expfile_mmap.path, strlen(expfile_mmap.path));
                ll_fs_write(f, &wr_u8, 1);

                tcb_floc = ll_fs_tell(f);
                wr_u8 = EXP_MAX_ALLOW_THREADS;
                ll_fs_write(f, &wr_u8, 1);
                //ll_fs_write(f, &tid, 1);
                for(int i = 0; i < EXP_MAX_ALLOW_THREADS; i++)
                {
                    //if(app_task_handle[i])
                    {
                        pre_save_tcb(app_task_handle[i]);
                        ll_fs_write(f, &app_tcb[i], sizeof(StaticTask_t));
                    }
                }
                
                mmapinfo_floc = ll_fs_tell(f);
                
                char *mf_path_buf = calloc(1, 128);
                assert(mf_path_buf);

                wr_u8 = EXP_MAX_ALLOW_MMAPS;
                ll_fs_write(f, &wr_u8, 1);
                for(int i = 0; i < EXP_MAX_ALLOW_MMAPS; i++)
                {
                    ll_fs_write(f, &exp_mmap_info[i], sizeof(mmap_info));
                    if(exp_mmap_info[i].path)
                    { 
                        strcpy(mf_path_buf, exp_mmap_info[i].path);
                        ll_fs_write(f, mf_path_buf, 128);
                    }else{
                        ll_fs_seek(f, 128, SEEK_CUR);
                    }
                }
                
                free(mf_path_buf);

                mem_floc = ll_fs_tell(f);
                ll_fs_write(f, &exp_mem_size, 4);
                ll_fs_write(f, (void *)APP_RAM_MAP_ADDR, exp_mem_size);
                


                scr_floc = ll_fs_tell(f);
                char *linebuf = calloc(1, 256);
                assert(linebuf);

                for(int y = 0; y < 127; y++)
                {
                    for(int x = 0; x < 256; x++)
                    {
                        linebuf[x] = bsp_display_get_point(x,y);
                    }
                    ll_fs_write(f, linebuf, 256);
                }

                mem_bitmap_floc = ll_fs_tell(f);
                for(int i = APP_RAM_MAP_ADDR; i < APP_RAM_MAP_ADDR + exp_mem_size; i+=1024)
                {
                    wr_u8 = ll_vaddr_dirty(i);
                    ll_fs_write(f, &wr_u8, 1);
                }


                ll_fs_seek(f, 8, SEEK_SET);
                ll_fs_write(f, &tcb_floc, 4);
                ll_fs_write(f, &mmapinfo_floc, 4);
                ll_fs_write(f, &mem_floc, 4);
                ll_fs_write(f, &scr_floc, 4);
                ll_fs_write(f, &mem_bitmap_floc, 4);
 
                ll_fs_close(f);

                free(linebuf);
                free(f);
                free(save_path);

                app_stop();
                
                xTaskResumeAll();
 
                app_selector_start();
            } 
        }
        if((_ON_LongPress == 30) && bsp_is_key_down(KEY_ON))
        {
            if(app_is_running())
            {
                _ON_LongPress = 0;
                app_stop();
                app_selector_start();
            }
        }
        if(bsp_is_key_down(KEY_ON))
        {
            _ON_LongPress++;
        }else{
            _ON_LongPress = 0;
        }
    }
}


void clear_key_queue();

//==============================


static void disp_puts(int x, int y, const char *s, int inv)
{
    bsp_display_putk_string(x*8,y*16, (char *)s, inv ? 255:0, inv ? 0:255);
}
static void bar_puts(int x, int y, const char *s, int inv)
{
    bsp_display_putk_string(x*8+3,y*16, (char *)s, inv ? 255:0, inv ? 0:255);
}
 
#define IKD(k)  bsp_is_key_down(k)

static uint32_t read_dir(file_list_t **flist, char *path)
{
    fs_dir_obj_t dirobj;
    uint32_t items = 0;
    clean_flist(flist);
    file_list_t *c;
    dirobj = calloc(1, ll_fs_get_dirobj_sz());
    ll_fs_dir_open(dirobj, path); 
    while (ll_fs_dir_read(dirobj) > 0)
    {
        items++;
        if(!*flist)
        {
            *flist = calloc(1, sizeof(file_list_t));
            (*flist)->next = NULL;
            (*flist)->fname = calloc(1, 1+strlen(ll_fs_dir_cur_item_name(dirobj)));
            (*flist)->type = ll_fs_dir_cur_item_type(dirobj);
            (*flist)->size = ll_fs_dir_cur_item_size(dirobj);
            strcpy((*flist)->fname, ll_fs_dir_cur_item_name(dirobj));
        }else{
            c = *flist;
            while(c->next)
            {
                 c = c->next;
            }
            c->next = calloc(1, sizeof(file_list_t));
            c = c->next;
            c->next = NULL;
            c->fname = calloc(1, 1+strlen(ll_fs_dir_cur_item_name(dirobj)));
            c->type = ll_fs_dir_cur_item_type(dirobj);
            c->size = ll_fs_dir_cur_item_size(dirobj);
            strcpy(c->fname, ll_fs_dir_cur_item_name(dirobj));
        }
    }
    ll_fs_dir_close(dirobj);
    free(dirobj);
    return items;
} 


static void draw_main(file_list_t *flist, char *path, int select)
{
    int y = 0, height = 6;
    int skip = select / height;
    bsp_diaplay_clean(0xFF);
    disp_puts(0,y,"Select an EXP to run:", 0);
    
    if(flist)
    {
        file_list_t *c = flist;
        while(skip){
            skip--;
            for(int i = 0; (i < height) && c; i++)
                c = c->next;
        }
        while (c)
        {
            if(c->type == FS_FILE_TYPE_REG)
            {
                disp_puts(0, 1+y, "[F]" , 0); 
            }else{
                disp_puts(0, 1+y, "[D]" , 0); 
            }
            disp_puts(4, 1+y, c->fname, y == (select % height)); 
            y++;
            if(y >= height)
                break;
            c = c->next;
        } 
    }
    bar_puts(0, 7, " RUN |    |PREV|NEXT|    |RET  ", 0);
}


static char* get_select_name(file_list_t *flist, int select)
{
    int i = 0;
    file_list_t *c = flist;
    while (c)
    {
        if(i == select)
            return c->fname;
        i++;
        c = c->next;
    } 
    return NULL;
}

static char *get_ext_name(char *path)
{
    
    uint32_t i = strlen(path) - 1;
    while (((path[i] != '/') && (path[i] != '.')) && i)
    {
        i--;
    }
    return &path[i];
}

static int get_select_type(file_list_t *flist, int select)
{
    int i = 0;
    file_list_t *c = flist;
    while (c)
    {
        if(i == select)
            return c->type;
        i++;
        c = c->next;
    } 
    return -1;
}

static void up_dir(char *path)
{
    int pathLen = strlen(path);
    if(pathLen > 1)
    {
        int i = pathLen - 2;
        while(path[i] != '/')
        {
            path[i] = 0;
            i--;
        }
    }
} 

static void load_sav(char *sav_path)
{
    
    printf("restore sav\r\n");
    tid = 0;
    char *org_path = calloc(1,128);
    fs_obj_t f = calloc(2, ll_fs_get_fobj_sz());
    StaticTask_t *tmp_ref_tcb = calloc(1, sizeof(StaticTask_t));
    assert(f);
    assert(tmp_ref_tcb);
    assert(org_path);

    ll_fs_open(f, sav_path, O_RDONLY);

    ll_fs_read(f, org_path, 8);
    if(!strcmp(org_path, "EXPSAVE"))
    {
        uint32_t tcb_floc, mmapinfo_floc, mem_floc, scr_floc, mem_bitmap_floc;
        uint32_t tmp_stack[4];

        vTaskSuspendAll();

        ll_fs_seek(f, 16*4, SEEK_SET);
        ll_fs_read(f, org_path, 128);

        expfile_mmap.map_to = APP_ROM_MAP_ADDR;
        expfile_mmap.offset = 0;
        expfile_mmap.writable = false;
        expfile_mmap.writeback = false;
        expfile_mmap.path = org_path;
        exp_self_mmap = ll_mmap(&expfile_mmap);

        ll_fs_seek(f, 8, SEEK_SET);
        ll_fs_read(f, &tcb_floc, 4);
        ll_fs_read(f, &mmapinfo_floc, 4);
        ll_fs_read(f, &mem_floc, 4);
        ll_fs_read(f, &scr_floc, 4);
        ll_fs_read(f, &mem_bitmap_floc, 4);

        printf("tcb_floc:%ld\r\n", tcb_floc);
        printf("mmapinfo_floc:%ld\r\n", mmapinfo_floc);
        printf("mem_floc:%ld\r\n", mem_floc);
        printf("scr_floc:%ld\r\n", scr_floc);

        uint8_t u8_rd = 0;
        ll_fs_seek(f, tcb_floc, SEEK_SET);
        ll_fs_read(f, &u8_rd, 1);
        for(int i = 0; i < u8_rd; i++)
        {
            ll_fs_read(f, tmp_ref_tcb, sizeof(StaticTask_t));
            if(((uint32_t *)tmp_ref_tcb)[0])
            {
                app_task_handle[tid] = xTaskCreateStatic(NULL, "NULL", 2, NULL, 1, tmp_stack, &app_tcb[tid]);
                tcb_restore((TaskHandle_t)tmp_ref_tcb, app_task_handle[tid]);
                tid++;
            }
        }

        ll_fs_seek(f, mmapinfo_floc, SEEK_SET);
        ll_fs_read(f, &u8_rd, 1);
        for(int i = 0; i < u8_rd; i++)
        {
            ll_fs_read(f, &exp_mmap_info[i], sizeof(mmap_info));
            if(exp_mmap_info[i].path)
            {
                exp_mmap_info[i].path = calloc(1, 128);
                assert(exp_mmap_info[i].path);

                ll_fs_read(f, (void *)exp_mmap_info[i].path, 128);
                
                exp_mmap_handle[i] = ll_mmap(&exp_mmap_info[i]);
            }else{
                ll_fs_seek(f, 128, SEEK_CUR);
            }
        }
 
        ll_fs_seek(f, scr_floc, SEEK_SET);
        char *linebuf = calloc(1, 256);
        assert(linebuf);

        for(int y = 0; y < 127; y++)
        {
            ll_fs_read(f, linebuf, 256);
            bsp_diaplay_put_hline(y, linebuf);
        }

        free(linebuf);

        uint32_t memsz = 0;
        ll_fs_seek(f, mem_floc, SEEK_SET);
        ll_fs_read(f, &memsz, 4);

        printf("Restore mem sz:%08lX\r\n", memsz);

        //exp_mem_wrap_alloc = malloc(memsz + 1*1024);
        //assert(exp_mem_wrap_alloc);
        //exp_mem_wrap_at = (uint32_t)exp_mem_wrap_alloc;
        //while((exp_mem_wrap_at % 1024))
        //{
        //    exp_mem_wrap_at++;
        //}  
        //exp_mem_size = (memsz/1024);
        //ll_set_exp_mem_wrap(exp_mem_wrap_at, exp_mem_size);
        //exp_mem_size = exp_mem_size * 1024;

        
        exp_mem_wrap_at = (uint32_t)&app_ram;
        exp_mem_size = memsz / 1024;
        ll_set_exp_mem_wrap(exp_mem_wrap_at, exp_mem_size);
        exp_mem_size = exp_mem_size * 1024;
        printf("Reload allocate App Mem:%ld\r\n", exp_mem_size);


        for(int i = 0; i < memsz / 1024; i++)
        {
            ll_fs_seek(f, mem_bitmap_floc + i, SEEK_SET);
            ll_fs_read(f, &u8_rd, 1);
            if(u8_rd)
            {
                ll_fs_seek(f, mem_floc + 4 + (i * 1024), SEEK_SET);
                ll_fs_read(f, (void *)(APP_RAM_MAP_ADDR + i*1024), 1024);
            }
        }
        //ll_fs_read(f, (void *)APP_RAM_MAP_ADDR, memsz);


        xTaskResumeAll();
    }else{
        free(org_path);
    }


    ll_fs_close(f);
    free(f);
    free(tmp_ref_tcb);

}

void app_selector(void *__)
{
    int sel = 0, rem = 0, pg = 0;
    int items;
    char cwd[256]; 
    char select_path[256];

    strcpy(cwd, "/");

    items = read_dir(&app_sel_flist, cwd);
    
    draw_main(app_sel_flist, cwd, sel);
     
    while (1)
    {
        if(IKD(KEY_DOWN))
        { 
            if(sel < items - 1)
                sel++;
            draw_main(app_sel_flist, cwd, sel);
        }
        else if(IKD(KEY_UP))
        {  
            if(sel)
                sel--;
            draw_main(app_sel_flist, cwd, sel);
        }else if(IKD(KEY_F4))
        {  
            sel += 6;
            if(sel >= items)
            {
                sel = items - 1;
            }
            draw_main(app_sel_flist, cwd, sel);
        }else if(IKD(KEY_F3))
        {  
            sel -= 6;
            if(sel < 0)
                sel = 0;
            draw_main(app_sel_flist, cwd, sel);
        } else if(IKD(KEY_ENTER) || IKD(KEY_F1))
        {
            if(get_select_type(app_sel_flist, sel) == FS_FILE_TYPE_DIR)
            {
                if(!strcmp(get_select_name(app_sel_flist,sel), "."))continue;
                else if(!strcmp(get_select_name(app_sel_flist,sel), ".."))up_dir(cwd);
                else{
                    if(cwd[strlen(cwd) - 1] != '/')
                    {
                        strcat(cwd, "/"); 
                    }
                    strcat(cwd, get_select_name(app_sel_flist,sel)); 
                }
                printf("cwd:%s\r\n", cwd);
                items = read_dir(&app_sel_flist, cwd);
                sel = 0;
                draw_main(app_sel_flist, cwd, sel);
            }else if(get_select_type(app_sel_flist, sel) == FS_FILE_TYPE_REG) {
                strcpy(select_path, cwd);
                if(select_path[strlen(select_path) - 1] != '/')
                {
                    strcat(select_path, "/"); 
                }
                strcat(select_path, get_select_name(app_sel_flist,sel));
                printf("sel:%S\r\n", select_path);
                printf("typ:%s\r\n", get_ext_name(select_path));
                
                if(!strcmp(get_ext_name(select_path), ".exp"))
                {
                    clear_key_queue();
                    app_stop();
                    app_pre_start(select_path, false, 0);
                    app_start();
                    clean_flist(&app_sel_flist);
                    app_selector_handle = NULL;
                    vTaskDelete(NULL);
                } else if(!strcmp(get_ext_name(select_path), ".sav"))
                {
                    clear_key_queue();
                    load_sav(select_path);
                    clean_flist(&app_sel_flist);
                    app_selector_handle = NULL;
                    vTaskDelete(NULL);
                }

                //if(!strcmp(get_select_name(app_sel_flist, sel), "test.txt"))
                //{
                //    fs_obj_t f = malloc(ll_fs_get_fobj_sz());
                //    int ret = ll_fs_open(f, select_path, O_RDWR | O_TRUNC);
                //    printf("test op:%d\r\n", ret);
                //    ll_fs_write(f, "Hello Test 12345", sizeof("Hello Test 12345"));
                //    ll_fs_close(f);
                //    free(f);
                //}
            }

        }else if(IKD(KEY_F6))
        {
            up_dir(cwd);
            printf("cwd:%s\r\n", cwd);
            items = read_dir(&app_sel_flist, cwd);
            sel = 0;
            draw_main(app_sel_flist, cwd, sel);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
//==============================
 
void app_api_init()
{ 

    memset(app_tcb, 0, sizeof(app_tcb));
    memset(app_task_handle, 0, sizeof(app_task_handle));
    memset(exp_mmap_handle, 0, sizeof(exp_mmap_handle));
    memset(exp_mmap_info, 0, sizeof(exp_mmap_info));
     
    app_api_queue = xQueueCreate(10, sizeof(app_api_info_t));
    xTaskCreate(app_api_task, "LLAPI Srv", 400, NULL, configMAX_PRIORITIES - 3, NULL); 

    app_selector_start();

}



