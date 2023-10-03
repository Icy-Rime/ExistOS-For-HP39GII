#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//#include <time.h>
#include "llapi.h"

#include "mem.h"

#define ROM_NAME    "cm.bin"

uint8_t *SRAM = NULL;
uint8_t *SROM = NULL;

#define ROM_MMAP_ADDR       (0x04000000)

uint32_t ROMSZ = 0;

extern uint32_t CurPC;



int memoryVirtAddrRead(uint32_t rd_address, uint32_t sz, void *data)
{
    uint32_t address = rd_address;
    uint32_t seg = (address & 0xF0000000);
    void *p = NULL;
    address &= 0x0FFFFFFF;

    uint32_t tmp = 0;

    switch (seg)
    {
    case (MEMORY_BASE):
    {
        if (address < MEMORY_SIZE)
        {
            //printf("adr:%08x\n", address);
            p = &SRAM[address];
        }
        else
        {
            printf("RD OOM:%08x at %08x\n", address, CurPC);
            return -1;
        }
        break;
    }

    case ROM_BASE:
    {
        if (address < ROMSZ)
            p = (void *)(ROM_MMAP_ADDR + address);
        else{
            printf("ROM OOM:%08x at %08x\n", address, CurPC);
            return -1;
        }
        break;
    }

    case 0xE0000000:
    {
        switch (address)
        {
        case 0x09000000+(1*4):
            tmp = 0;
            p = &tmp;
            break;
        case 0x09000000+(2*4):
            tmp = 1;
            p = &tmp;
            break;

        case 0x0FFF000c:
            tmp = (llapi_get_tick_ms());
            p = &tmp;
            break;
        case 0x0EEE000c:
            tmp = (llapi_get_tick_ms());
            p = &tmp;
            break;
        default:
            printf("RD P ERR: %08x at: %08x\n", rd_address, CurPC);
            return -1;
        }
        break;
    }

    default:
        printf("RD ERR: %08x at: %08x\n", rd_address, CurPC);
        return -1;
    }

    switch (sz)
    {
    case 1:
        *((uint8_t *)data) = *((uint8_t *)p);
        return 0;
    case 2:
         if (address % 2)
         {
             printf("RD2 NOT Align:%08x at %08x\n", rd_address, CurPC);
             return -1;
         }
        *((uint16_t *)data) = *((uint16_t *)p);
        return 0;
    case 4:
        if (address % 4)
         {
             printf("RD4 NOT Align:%08x at %08x\n", rd_address, CurPC);
             return -1;
         }
         uint32_t readdat = *((uint32_t *)p);
        *((uint32_t *)data) = readdat;
        return 0;
    default:
        printf("RD ERROR SZ!: %08x,%08x at %08x\n", rd_address, *((uint32_t *)data),CurPC);
        return -1;
    }
    return 0;
}

int memoryVirtAddrWrite(uint32_t wr_address, uint32_t sz, uint32_t data)
{

    uint32_t address = wr_address;
    uint32_t seg = address & 0xF0000000;
    void *p = NULL;
    address &= 0x0FFFFFFF;
    switch (seg)
    {
    case (MEMORY_BASE):
    {
        if (address < MEMORY_SIZE)
        {
            p = (uint8_t *)SRAM + address;
        }
        else
        {
            return -1;
        }
        break;
    }
    case 0xE0000000:
    {
        switch (address)
        {
        case 0x09000000+(0*4):
        case 0x0fff0004:
            if (data)
                putchar(data);
            return 0;
        
        case 0x0fff0004+(5*4):

            return 0;
        default:
            printf("WR ERROR SZ!: %08x,%08x at %08x\n", wr_address, data, CurPC);
            return -1;
        }
    }

    default:
        printf("WR ERROR SZ!: %08x,%08x at %08x\n", wr_address, data, CurPC);
        return -1;
    }

    switch (sz)
    {
    case 1:
        *((uint8_t *)p) = data;
        return 0;
    case 2:
        if (address % 2)
         {
             printf("WR2 NOT Align:%08x at %08x\n", wr_address, CurPC);
             return -1;
         }
        *((uint16_t *)p) = data;
        return 0;
    case 4:
        if (address % 4)
         {
             printf("WR4 NOT Align:%08x at %08x\n", wr_address, CurPC);
             return -1;
         }
        *((uint32_t *)p) = data;
        return 0;
    default:
        printf("WR ERROR SZ!: %08x,%08x at %08x\n", wr_address, data, CurPC);
        return -1;
    }
    return 0;
}

void dumpMem()
{
    for (int i = 0; i < 12; i++)
    {
        printf("%08x\n", SRAM[i]);
    }
}

#include "fcntl.h"

uint32_t get_rom_sz()
{
    uint32_t sz = 0;
    fs_obj_t f = malloc(llapi_fs_get_fobj_sz());
    int ret = llapi_fs_open(f, ROM_NAME, O_RDONLY);
    if(ret)
    {
        free(f);
        return 0;
    }
    sz = llapi_fs_size(f);
    llapi_fs_close(f);
    free(f);
    return sz;
}

int memoryInit()
{
    SRAM = calloc(1, MEMORY_SIZE); 
    SROM = (uint8_t *)ROM_MMAP_ADDR;
    if( ROMSZ = get_rom_sz())
    {
        mmap_info mf;
        mf.map_to = ROM_MMAP_ADDR;
        mf.offset = 0;
        mf.path = ROM_NAME;
        mf.size = 0;
        mf.writable = false;
        mf.writeback = false;
        int ret = llapi_mmap(&mf);
        printf("mmap rom:%d sz: %ld\r\n", ret, ROMSZ);
    }else{
        printf("MMAP ERROR\r\n");
    }
    return 0;
}
