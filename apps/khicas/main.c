#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "llapi.h"


 
void start_khicas();
void vGL_FlushVScreen();

void task_flush_disp()
{
    printf("task_flush_disp\r\n");
    while(1)
    {
        vGL_FlushVScreen();
        llapi_delay_ms(30);
    }    
}

int main()
{
    printf("Start App..\r\n");
    llapi_thread_create(task_flush_disp, malloc(512),512,NULL);

    start_khicas();

    while (1)
    {
        printf("exit...\r\n");
        llapi_delay_ms(1000);
        /* code */
    }

}

