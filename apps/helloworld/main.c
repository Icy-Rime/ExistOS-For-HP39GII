#include <stdio.h>
#include <string.h>
 
#include "llapi.h"

int main()
{
    printf("Start App..\r\n"); 

    while (1)
    {
        printf("Hello world.\r\n");
        llapi_delay_ms(1000); 
    }

}

