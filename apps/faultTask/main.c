#include <stdio.h>
#include <string.h>
 
#include "llapi.h"


  
 
int main()
{
    printf("Start App..\r\n"); 

    llapi_fs_dir_mkdir("/exp");
    llapi_fs_dir_mkdir("/exp/testa");
    llapi_fs_dir_mkdir("/exp/testa/testb");

    //((uint32_t *)0x12340000)[0] = 1;
    for(int y = 0; y < 64; y++)
    {
        for(int x = 0; x < 256; x++)
        {
             llapi_disp_put_point(x, y, x & 0xF8);
        }
    }
    
    for(int y = 65; y < 127; y++)
    {
        for(int x = 0; x < 256; x++)
        {
             llapi_disp_put_point(x, y, x);
        }
    }


    while (1)
    {
        printf("loop...\r\n");
        llapi_delay_ms(1000); 
    }

}

