#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "llapi.h"
#include "keys_39gii.h"

#include "gb/timer.h"
#include "gb/rom.h"
#include "gb/mem.h"
#include "gb/cpu.h"
#include "gb/lcd.h"
#include "gb/sdl.h"

typedef struct file_list_t
{
    struct file_list_t *next;
    char *fname;
    int type;
    uint32_t size;
}file_list_t;

file_list_t *flist = NULL;

//==============================

static void disp_puts(int x, int y, const char *s, int inv)
{
    llapi_disp_put_kstr(x*8,y*16, (char *)s, inv ? (0xFF<<16):0xFF);
}
static void bar_puts(int x, int y, const char *s, int inv)
{
    llapi_disp_put_kstr(x*8+3,y*16, (char *)s, inv ? (0xFF<<16):0xFF);
}
 
#define IKD(k)  llapi_is_key_down(k)

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
    dirobj = calloc(1, llapi_fs_get_dirobj_sz());
    llapi_fs_dir_open(dirobj, path); 
    while (llapi_fs_dir_read(dirobj) > 0)
    {
        items++;
        if(!*flist)
        {
            *flist = calloc(1, sizeof(file_list_t));
            (*flist)->next = NULL;
            (*flist)->fname = calloc(1, 1+strlen(llapi_fs_dir_cur_item_name(dirobj)));
            (*flist)->type = llapi_fs_dir_cur_item_type(dirobj);
            (*flist)->size = llapi_fs_dir_cur_item_size(dirobj);
            strcpy((*flist)->fname, llapi_fs_dir_cur_item_name(dirobj));
        }else{
            c = *flist;
            while(c->next)
            {
                 c = c->next;
            }
            c->next = calloc(1, sizeof(file_list_t));
            c = c->next;
            c->next = NULL;
            c->fname = calloc(1, 1+strlen(llapi_fs_dir_cur_item_name(dirobj)));
            c->type = llapi_fs_dir_cur_item_type(dirobj);
            c->size = llapi_fs_dir_cur_item_size(dirobj);
            strcpy(c->fname, llapi_fs_dir_cur_item_name(dirobj));
        }
    }
    llapi_fs_dir_close(dirobj);
    free(dirobj);
    return items;
} 


static void draw_main(file_list_t *flist, char *path, int select)
{
    int y = 0, height = 6;
    int skip = select / height;
    llapi_disp_clean(0xFF);
    disp_puts(0,y,"Select an ROM:", 0);
    
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

void emu_loop()
{
    int r=0;
    while(1)
    {
    	int now;    
    	if(!cpu_cycle())
    		break;  
    	now = cpu_get_cycles(); 
    	while(now != r)
    	{
    		int i;  
    		for(i = 0; i < 4; i++)
    			if(!lcd_cycle())
    				return;   
    		r++;
    	}   
    	timer_cycle();  
    	r = now;
    }
}

void app_selector(void *__)
{
    int sel = 0, rem = 0, pg = 0;
    int items;
    char cwd[256]; 
    char select_path[256];

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
            while(IKD(KEY_ENTER) || IKD(KEY_F1))
                ;
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
                
                int r = 0;
                r = rom_load(select_path);
	            if(!r)
                    continue;

	            r = lcd_init();
	            if(r)
	            	continue;

	            printf("LCD OK!\n");

	            mem_init();
	            printf("Mem OK!\n");

	            cpu_init();
	            printf("CPU OK!\n");
                emu_loop();
	            
                llapi_delay_ms(200);

            }

        }else if(IKD(KEY_F6))
        {
            up_dir(cwd);
            printf("cwd:%s\r\n", cwd);
            items = read_dir(&flist, cwd);
            sel = 0;
            draw_main(flist, cwd, sel);
        }
        llapi_delay_ms(50);
    }
}
//==============================


int main()
{

    printf("Start App..\r\n"); 

    app_selector(NULL);
    while (1)
    { 
        llapi_delay_ms(1000); 
    }

}

