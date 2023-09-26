
#include "decode.h"
#include "mem.h"
#include "opcode_str.h"

extern DecodeBlock DBs[MAX_DECODE_BLOCKS];

void dump_IRCode(IRCode INS)
{
    
    printf("OPCODE:  %s,", opcode_str[INS.opcode]);

    printf("%02lu, %02lu, %02lu, %08lx\n",
           (unsigned long int)INS.rd,
           (unsigned long int)INS.rs1,
           (unsigned long int)INS.rs2,
           (unsigned long int)INS.imm);
}

void dump_DB(uint32_t src_pc)
{
    DecodeBlock *DB;
    uint32_t offset;
    GetCachedDBByAddr(src_pc, &DB, &offset);
    if (DB)
    {
        printf("DB SRC PC:%d\n", DB->src_pc);
        printf("DB SRC LEN:%d\n", DB->src_codes_length);
        printf("DB IR_Length:%d\n", DB->IR_Length);
        for (int i = 0; i < DB->IR_Length; i++)
        {
            dump_IRCode(DB->Ins[i]);
        }
    }
}

void dump_all_DB()
{
    for (int j = 0; j < MAX_DECODE_BLOCKS; j++)
    {
        if (DBs[j].src_codes_length > 0)
        {
            printf("%d:\n", j);
            printf("\tDB SRC PC:%d\n", DBs[j].src_pc);
            printf("\tDB SRC LEN:%d\n", DBs[j].src_codes_length);
            printf("\tDB IR_Length:%d\n", DBs[j].IR_Length);

            for (int i = 0; i < DBs[j].IR_Length; i++)
            {
                dump_IRCode(DBs[j].Ins[i]);
            }
        }
    }
}
