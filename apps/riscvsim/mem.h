#pragma once

#define MEMORY_BASE  (0x80000000)
#define MEMORY_SIZE  (64 * 1024)

#define ROM_BASE     (0x70000000) 



void dumpMem();
int memoryInit();

int memoryVirtAddrRead(uint32_t address, uint32_t sz,void *data);
int memoryVirtAddrWrite(uint32_t address, uint32_t sz, uint32_t data);
