#include <stdio.h>
#include <string.h>
 
#include "llapi.h"

#include "mem.h"
#include "simulator.h"
#include "decode.h"

  
 
int main()
{
    printf("Start App..\r\n");  
    
    memoryInit();
    DecodeInit();
    
    simulatorStart(ROM_BASE);

    while (1)
    {
        printf("loop...\r\n");
        llapi_delay_ms(1000); 
    }

}

