 
#include "string.h"
#include "stdio.h"

#include "llapi.h"

#define LOAD_BLOCK_SIZE 65536

void *emu_rom_addr = NULL;

static size_t rom_size;

static bool rom_isPacked = true;

static uint8_t *rdrom_buf;

bool rom_is_packed()
{
    return rom_isPacked;
}

static mmap_info rom_map;

int load_rom(const char *romfile) {

    emu_rom_addr = (void *)0x30000000;
    rom_map.map_to = 0x30000000;
    rom_map.offset = 0;
    rom_map.path = romfile;
    rom_map.size = 0;
    rom_map.writable = false;
    rom_map.writeback = false;
    rom_size = 2097152/2;
    return llapi_mmap(&rom_map);
}

//static uint32_t maxsz = 0;
#if 0
static uint8_t data_packed[32];
static uint8_t data_unpacked[sizeof(data_packed) * 2];

uint8_t rom_read_nibbles(void *dst,uint32_t nibble_addr, uint32_t size)
{
    if(nibble_addr >= 0x30000000)
    {
        //if(nibble_addr < 0x70000000 + rom_size * 2)
        {
            uint32_t acc_addr = nibble_addr - 0x30000000;

            memcpy(data_packed, (void *)(acc_addr / 2 + emu_rom_addr) , sizeof(data_packed));

            for(int i =0, j =0; i < sizeof(data_packed); i++)
            {
                data_unpacked[j++] = data_packed[i] & 0xF;
                data_unpacked[j++] = data_packed[i] >> 4;
            }
            memcpy(dst, &data_unpacked[nibble_addr & 1] , size);

/*
            if(nibble_addr & 1)
            {
                memcpy(dst, &data_unpacked[1] , size);
            }else
            {
                memcpy(dst, data_unpacked , size);
            }
            */

            //memcpy(dst, (void *)(acc_addr + emu_rom_addr) , size);

/*
            if(size > maxsz)
            {
                maxsz = size;
                printf("rddmsz:%d\n", size);
            }
            */

            return 0;
        }
    }else{
        memcpy(dst, (void *)nibble_addr, size);
    }


    //printf("RD ERR:%08x %08x\n", nibble_addr, emu_rom_addr);
}

#endif



void *get_rom_addr() {
    return emu_rom_addr;
}

size_t get_rom_size() {
    return rom_size;
}
