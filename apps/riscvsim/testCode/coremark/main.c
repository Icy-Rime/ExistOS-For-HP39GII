#include <stdint.h>
#include <stdio.h>
#include "LLLapi.h"



//int siprintf(char *buf, const char *fmt, ...);
//char buf[512];

int cm_main();

int main()
{    
    int t = 0;
    
    printf("test:%d\r\n", t); 
    while(1)
    {
        delay(1000);
        printf("test:%d\r\n", t); 
        cm_main();
        t++;
    }
}

