#include <stdio.h>
#include <string.h>
 
#include "llapi.h"


 
void start_khicas();
 
int main()
{
    printf("Start App..\r\n");
    start_khicas();

    while (1)
    {
        printf("exit...\r\n");
        llapi_delay_ms(1000);
        /* code */
    }

}

