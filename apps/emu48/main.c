#include <stdio.h>
#include <string.h>
 
#include "llapi.h"

void emu48_main(int select);

int main()
{
    printf("Start App..\r\n"); 
    emu48_main(0);
    while (1)
    {
        printf("Hello world.\r\n");
        llapi_delay_ms(1000); 
    }

}

