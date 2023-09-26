#include <fcntl.h>

#include "app_runtime.h"
#include "queue.h"
#include "../apps/llapi.h"

QueueHandle_t app_api_queue;

static void *app_mem_warp_alloc = NULL;
static uint32_t app_mem_warp_at = 0;
static uint32_t app_mem_size = 0;
static volatile bool app_running = false;
static StaticTask_t app_tcb;
static TaskHandle_t app_task_handle = NULL;
static int mmap = 0;

//static char mmaps[6];
uint32_t app_stack_sz = 0; 


bool app_is_running()
{
    return app_running;
}



uint32_t app_get_ram_size()
{
    return app_mem_size;
}


static void app_main_thread(void *par) {

    __asm volatile("mrs r1,cpsr_all");
    __asm volatile("bic r1,r1,#0x1f");
    __asm volatile("orr r1,r1,#0x10");
    __asm volatile("msr cpsr_all,r1"); 

    __asm volatile("ldr r1,=%0" : : ""(APP_ROM_MAP_ADDR + 32) : );
    __asm volatile("bx r1");

    vTaskDelete(NULL);
    while (1) { 
        vTaskDelay(pdMS_TO_TICKS(1000));
        ;
    }
}


mmap_info mf;

void app_stop()
{
    
    if(!app_running)
        return;
    if(!app_task_handle)
        return;

    vTaskSuspendAll();

    free((void *)mf.path);


    vTaskDelete(app_task_handle);

    void sls_unlock_all();
    sls_unlock_all();
    ll_set_app_mem_warp(0, 0);
    free(app_mem_warp_alloc);
    app_mem_warp_alloc = NULL;

    app_running = false;
    app_task_handle = NULL;

    //for(int i = 0; i < sizeof(mmaps); i++)
    //{
    //    if(mmaps[i])
    //        ll_mumap(mmaps[i]);
    //    mmaps[i] = 0;
    //}
    ll_mumap(mmap);
    for(int i = 1; i < 6; i++)
    {
        ll_mumap(i);
    }

    for(uint32_t i = APP_RAM_MAP_ADDR; i < APP_RAM_MAP_ADDR + app_mem_size; i+=1024)
    {
        ll_mm_trim_vaddr(i & ~(1023));
    }

    xPortHeapTrimFreePage();
    app_mem_size = 0;
    xTaskResumeAll();
    vTaskDelay(pdMS_TO_TICKS(50));
}

static volatile uint32_t rdVal = 0;
static void app_rom_read(void *p)
{
    vTaskDelay(pdMS_TO_TICKS(100));
    rdVal = ((uint32_t *)p)[0];
    vTaskDelay(pdMS_TO_TICKS(100));
    vTaskDelete(NULL);
}

void app_start()
{
    if(app_running)
    {
        return;
    }
    if(mmap <= 0)
    {
        return;
    }

    bsp_diaplay_clean(0xFF); 

    uint32_t *stackSz = (uint32_t *)(APP_ROM_MAP_ADDR + 8);
    //TaskHandle_t task_app_rom_read;
    //xTaskCreate(app_rom_read, "app_rom_read", 32, stackSz, 2, &task_app_rom_read);
    //vTaskDelay(pdMS_TO_TICKS(200));
    //int i = 0;
    //while (rdVal == 0)
    //{
    //    vTaskDelay(pdMS_TO_TICKS(500));
    //    i++;
    //    if(i > 10)
    //    {
    //        break;
    //    }
    //}
    if(*stackSz != 0xE1A00000)
    {
        app_stack_sz = *stackSz;
        printf("SET Stack Size:%ld\r\n", app_stack_sz );
    }

    if(app_stack_sz == 0)
    {
        app_stack_sz = 400;
    }

    vTaskSuspendAll();
    uint32_t free_mem = getFreeMemSz() - 12*1024;
    app_mem_warp_alloc = malloc(free_mem);
    app_mem_warp_at = (uint32_t)app_mem_warp_alloc;
    while((app_mem_warp_at % 1024))
    {
        app_mem_warp_at++;
    }  
    app_mem_size = (free_mem/1024) - 1;
    ll_set_app_mem_warp(app_mem_warp_at, app_mem_size);
    app_mem_size = app_mem_size * 1024;

    printf("Allocate App Mem:%ld\r\n", app_mem_size);
     app_task_handle = xTaskCreateStatic(
         app_main_thread, 
          "app_t0", 
          app_stack_sz, 
          NULL, 1,
          (StackType_t *const)(APP_RAM_MAP_ADDR  + app_mem_size - app_stack_sz * 4 - 4) , &app_tcb); //APP_RAM_MAP_ADDR  + app_mem_size - 4 - app_stack_sz * 4
    //xTaskCreate(app_main_thread, "app_t0", 400, NULL, 1, &app_task_handle);
    
    gdb_attach_to_task(app_task_handle);
    xTaskResumeAll();
    app_running = true;
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
    
    app_stop();
    
    mf.map_to = APP_ROM_MAP_ADDR;
    mf.offset = 0;
    mf.writable = false;
    mf.writeback = false;
    if(sideload)
    {
        mf.size = sideload_sz;
        mf.path = calloc(1, 3);
        memcpy((void *)mf.path,(void *)"\03",1);
    }else{
        mf.path = calloc(1, 64);
        strncpy((char *)mf.path, path, 63);
        uint32_t expSz = app_get_exp_sz(path);
        //int32_t remainSz = expSz;
        printf("app rom sz:%d\r\n", expSz);
        mf.size = 0;
        //mmaps[0] = ll_mmap(&mf);
        mmap = ll_mmap(&mf);
        printf("A mmap:%d, off:%08lX,sz:%ld,  %s\r\n", mmap, mf.offset, mf.size,  mf.path);
        //for(int i = 0; i <= expSz / 1048576; i++)
        //{
        //    mf.map_to = APP_ROM_MAP_ADDR + i * 1048576;
        //    mf.offset = i * 1048576;
        //    mf.size = 1048576;
        //    remainSz -= mf.size;
        //    mmaps[i] = ll_mmap(&mf);
        //    printf("A mmap:%d, off:%08lX,sz:%ld,  %s\r\n", mmaps[i],mf.offset,mf.size,  mf.path);
        //    if((i == 1) || ((remainSz < 1048576) && (remainSz > 0)))
        //    {
        //        i++;
        //        mf.map_to = APP_ROM_MAP_ADDR + i * 1048576;
        //        mf.offset = i * 1048576;
        //        mf.size = 1048576;
        //        mf.size = remainSz;
        //        mmaps[i] = ll_mmap(&mf);
        //        printf("B mmap:%d, off:%08lX,sz:%ld,  %s\r\n", mmaps[i],mf.offset,mf.size,  mf.path);
        //        break;
        //    }
        //}

        
    }

    //mmap = ll_mmap(&mf);
    
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
                case LLAPI_APP_DELAY_MS:
                    vOtherTaskDelay(info.task, pdMS_TO_TICKS(info.par0));
                    break;
                case LLAPI_APP_GET_KEY:
                    vTaskSuspend(info.task);
                    break;

            default:
                break;
            }
        }
    }
}




//==============================

typedef struct file_list_t
{
    struct file_list_t *next;
    char *fname;
    int type;
    uint32_t size;
}file_list_t;

static void disp_puts(int x, int y, const char *s, int inv)
{
    bsp_display_putk_string(x*8,y*16, (char *)s, inv ? 255:0, inv ? 0:255);
}
static void bar_puts(int x, int y, const char *s, int inv)
{
    bsp_display_putk_string(x*8+3,y*16, (char *)s, inv ? 255:0, inv ? 0:255);
}
 
#define IKD(k)  bsp_is_key_down(k)

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
    bar_puts(0, 7, " RUN |    | UP |DOWN|    |RET  ", 0);
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


void app_selector(void *__)
{
    int sel = 0, rem = 0, pg = 0;
    int items;
    char cwd[256]; 
    char select_path[256];

    file_list_t *flist = NULL;

    strcpy(cwd, "/");

    items = read_dir(&flist, cwd);
    
    draw_main(flist, cwd, sel);
     
    while (1)
    {
        if(IKD(KEY_DOWN))
        { 
            if(sel < items - 1)
                sel++;
            draw_main(flist, cwd, sel);
        }
        else if(IKD(KEY_UP))
        {  
            if(sel)
                sel--;
            draw_main(flist, cwd, sel);
        }else if(IKD(KEY_F4))
        {  
            sel += 6;
            if(sel >= items)
            {
                sel = items - 1;
            }
            draw_main(flist, cwd, sel);
        }else if(IKD(KEY_F3))
        {  
            sel -= 6;
            if(sel < 0)
                sel = 0;
            draw_main(flist, cwd, sel);
        } else if(IKD(KEY_ENTER) || IKD(KEY_F1))
        {
            if(get_select_type(flist, sel) == FS_FILE_TYPE_DIR)
            {
                if(!strcmp(get_select_name(flist,sel), "."))continue;
                else if(!strcmp(get_select_name(flist,sel), ".."))up_dir(cwd);
                else{
                    if(cwd[strlen(cwd) - 1] != '/')
                    {
                        strcat(cwd, "/"); 
                    }
                    strcat(cwd, get_select_name(flist,sel)); 
                }
                printf("cwd:%s\r\n", cwd);
                items = read_dir(&flist, cwd);
                sel = 0;
                draw_main(flist, cwd, sel);
            }else if(get_select_type(flist, sel) == FS_FILE_TYPE_REG) {
                strcpy(select_path, cwd);
                if(select_path[strlen(select_path) - 1] != '/')
                {
                    strcat(select_path, "/"); 
                }
                strcat(select_path, get_select_name(flist,sel));
                printf("sel:%S\r\n", select_path);
                printf("typ:%s\r\n", get_ext_name(select_path));
                
                if(!strcmp(get_ext_name(select_path), ".exp"))
                {
                    clean_flist(&flist);
                    app_stop();
                    app_pre_start(select_path, false, 0);
                    app_start();

                    vTaskDelete(NULL);
                }
            }

        }else if(IKD(KEY_F6))
        {
            up_dir(cwd);
            printf("cwd:%s\r\n", cwd);
            items = read_dir(&flist, cwd);
            sel = 0;
            draw_main(flist, cwd, sel);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
//==============================
 

void app_api_init()
{ 
    app_api_queue = xQueueCreate(10, sizeof(app_api_info_t));
    xTaskCreate(app_api_task, "LLAPI Srv", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, NULL);
    xTaskCreate(app_selector, "app_selector", 400, NULL, configMAX_PRIORITIES - 4, NULL);
}



