

#include "decode.h"
#include "mem.h"

#define DB_CACHE_SOCKET (MAX_DECODE_BLOCKS)

DecodeBlock DBs[MAX_DECODE_BLOCKS];
DecodeBlock *DBCacheSlot[(MAX_DECODE_BLOCKS + 1)];

#define address_hash(x) ((x >> 2) & DB_CACHE_SOCKET)

int DecodeInit()
{
    memset(DBs, 0, sizeof(DBs));
    memset(DBCacheSlot, 0, sizeof(DBCacheSlot));
    printf("IR Cache SZ:%d\n", (uint32_t)sizeof(DBs));
    return 0;
}

int GetCachedDBByAddr(uint32_t src_pc, DecodeBlock **DB, uint32_t *offset)
{
    

    for (int i = 0; i < MAX_DECODE_BLOCKS; i++)
    {
        if (DBs[i].src_codes_length > 0)
        {
            if ((src_pc >= DBs[i].src_pc) && (src_pc < DBs[i].src_pc + DBs[i].src_codes_length))
            {
                *DB = &DBs[i];
                *offset = src_pc - DBs[i].src_pc;
                //DBs[i].exec_cnt++;
                return 0;
            }
        }
    }
    
    //uint32_t mask_i = address_hash(src_pc);
    //if (DBCacheSlot[mask_i])
    //{
    //    uint32_t lower = DBCacheSlot[mask_i]->src_pc;
    //    uint32_t upper = DBCacheSlot[mask_i]->src_pc + DBCacheSlot[mask_i]->src_codes_length;
    //    if ((src_pc >= lower) &&
    //        (src_pc < upper))
    //    {
    //        // printf("search:%08x, found:[%08x,%08x)\n",src_pc, lower, upper);
    //        *DB = DBCacheSlot[mask_i];
    //        *offset = src_pc - DBCacheSlot[mask_i]->src_pc;
    //        return 0;
    //    }
    //}

    *DB = NULL;
    *offset = 0;
    return -1;
}

DecodeBlock *PrepareNewDB(uint32_t src_pc)
{

    static uint32_t minimum_i = 0;
    minimum_i++;
    if (minimum_i == MAX_DECODE_BLOCKS)
        minimum_i = 0;

    //uint32_t minimum_i = 0;
    //uint32_t minimum_exec_cnt = -1;
    //for(int i = 0; i < MAX_DECODE_BLOCKS; i++)
    //{
    //    if(DBs[i].exec_cnt < minimum_exec_cnt)
    //    {
    //        minimum_exec_cnt = DBs[i].exec_cnt;
    //        minimum_i = i;
    //    }
    //}

    //DBCacheSlot[address_hash(src_pc)] = &DBs[minimum_i];

    DBs[minimum_i].src_pc = src_pc;
    DBs[minimum_i].IR_Length = 0;
    DBs[minimum_i].src_codes_length = 0;
    if(DBs[minimum_i].Ins)
    {
        free(DBs[minimum_i].Ins);
        DBs[minimum_i].Ins = NULL;
    }
    //DBs[minimum_i].exec_cnt = 1;
    return &DBs[minimum_i];
}

int DBInsertIRCode(DecodeBlock *DB,
                   IRCode IRC)
{



    IRCode *newirc = realloc(DB->Ins,(DB->IR_Length + 1) * sizeof(IRCode));
    if(!newirc)
    {
        printf("No mem!\r\n");
    }
    DB->Ins = newirc;
    DB->Ins[DB->IR_Length] = IRC;
    DB->IR_Length++;
    if(DB->IR_Length > 127)
       return 1;
    return 0;
    //if (INS_PER_DB - DB->IR_Length > 3)
    //{
    //    DB->Ins[DB->IR_Length] = IRC;
    //    DB->IR_Length++;
    //    return 0;
    //}
    //else // if (INS_PER_DB - DB->IR_Length > 0)
    //{
    //    DB->Ins[DB->IR_Length] = IRC;
    //    DB->IR_Length++;
    //    return 1;
    //}
     /*
     else
     {
         return -1;
     }*/
}

void UndIns(DecodeBlock *DB,
            IRCode IRC, uint32_t vaddr)
{
    uint32_t ins = -1;
    memoryVirtAddrRead(vaddr, 1, &((uint8_t *)&ins)[0]);
    memoryVirtAddrRead(vaddr + 1, 1, &((uint8_t *)&ins)[1]);
    memoryVirtAddrRead(vaddr + 2, 1, &((uint8_t *)&ins)[2]);
    memoryVirtAddrRead(vaddr + 3, 1, &((uint8_t *)&ins)[3]);

    printf("UND at %08x:%08x\n", vaddr, ins);

    exit(-1);
}

#define INSERT_UND_INS                    \
    {                                     \
        IRC.opcode = IR_OPCODE_UND;       \
        ret = DBInsertIRCode(curDB, IRC); \
        UndIns(curDB, IRC, curVaddr);     \
        break;                            \
    }

#define INSERT_REGOP_INS(funct, opc, rd, rs1, rs2) \
    case funct:                                    \
    {                                              \
        IRC.opcode = opc;                          \
        IRC.rd = rd;                               \
        IRC.rs1 = rs1;                             \
        IRC.rs2 = rs2;                             \
        ret = DBInsertIRCode(curDB, IRC);          \
        break;                                     \
    }

#define INSERT_REGIMM_INS(funct, opc, rd, rs1, imm) \
    case funct:                                     \
    {                                               \
        IRC.opcode = opc;                           \
        IRC.rd = rd;                                \
        IRC.rs1 = rs1;                              \
        IRC.imm = imm;                              \
        ret = DBInsertIRCode(curDB, IRC);           \
        break;                                      \
    }

#define INSERT_STORE_INS(funct, opc, rs1, rs2, imm) \
    case funct:                                     \
    {                                               \
        IRC.opcode = opc;                           \
        IRC.rs1 = rs1;                              \
        IRC.rs2 = rs2;                              \
        IRC.imm = imm;                              \
        ret = DBInsertIRCode(curDB, IRC);           \
        break;                                      \
    }

#define INSERT_BRUNCH_INS(funct, opc, rs1, rs2, to) \
    case funct:                                     \
    {                                               \
        IRC.opcode = opc;                           \
        IRC.rs1 = rs1;                              \
        IRC.rs2 = rs2;                              \
        IRC.imm = to;                               \
        ret = DBInsertIRCode(curDB, IRC);           \
        break;                                      \
    }

int DecodeSrc(uint32_t VirtAddr)
{

    IRCode IRC;

    uint32_t curIns = 0;
    uint32_t curVaddr = VirtAddr;
    DecodeBlock *curDB = PrepareNewDB(VirtAddr);
    curDB->src_ins_len = 4;

    int ret;
    int st = 1;
    int compress = 0;

    uint32_t opcode;
    uint32_t funct3;
    uint32_t funct7 = 0;
    uint32_t rd = 0;
    uint32_t rs1 = 0;
    uint32_t rs2 = 0;

    // curIns = memoryVirtAddrRead16(curVaddr);
    memoryVirtAddrRead(curVaddr, 2, &curIns);
    if ((curIns & 0b11) != 0b11)
    {
        compress = 1;
        curDB->src_ins_len = 2;
    }

    while (st)
    {

        memoryVirtAddrRead(curVaddr, 2, &curIns);

        if ((curIns & 0b11) != 0b11)
        {
            if (compress == 0) // C to NC
            {
                return 0;
            }
            opcode = curIns & 0b11;
            funct3 = (curIns >> 13) & 0b111;
        }
        else
        {
            if (compress == 1) // NC to C
            {
                return 0;
            }
            memoryVirtAddrRead(curVaddr + 2, 2, &((uint16_t *)&curIns)[1]);
            opcode = curIns & 0x7F;
            funct3 = (curIns >> 12) & 7;
            funct7 = (curIns >> 25) & 0xFF;
            rd = ((curIns >> 7) & 0x1F);
            rs1 = ((curIns >> 15) & 0x1F);
            rs2 = ((curIns >> 20) & 0x1F);
        }

        // memset(&IRC, 0, sizeof(IRC));

        switch (opcode)
        {
        case OPCODE_R_TYPE_1:
        {
            if (funct7 == 0)
            {
                switch (funct3)
                {
                    INSERT_REGOP_INS(0x0, IR_OPCODE_ADD, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x4, IR_OPCODE_XOR, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x6, IR_OPCODE_OR, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x7, IR_OPCODE_AND, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x1, IR_OPCODE_SLL, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x5, IR_OPCODE_SRL, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x2, IR_OPCODE_SLT, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x3, IR_OPCODE_SLTU, rd, rs1, rs2);
                default:
                    INSERT_UND_INS;
                    break;
                }
            }
            else if (funct7 == 0x20)
            {
                switch (funct3)
                {
                    INSERT_REGOP_INS(0x0, IR_OPCODE_SUB, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x5, IR_OPCODE_SRA, rd, rs1, rs2);
                default:
                    INSERT_UND_INS;
                    break;
                }
            }
            else if (funct7 == 0x01)
            {
                switch (funct3) // RV32M
                {
                    INSERT_REGOP_INS(0x0, IR_OPCODE_MUL, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x1, IR_OPCODE_MULH, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x2, IR_OPCODE_MULHSU, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x3, IR_OPCODE_MULHU, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x4, IR_OPCODE_DIV, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x5, IR_OPCODE_DIVU, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x6, IR_OPCODE_REM, rd, rs1, rs2);
                    INSERT_REGOP_INS(0x7, IR_OPCODE_REMU, rd, rs1, rs2);

                default:
                    INSERT_UND_INS;
                    break;
                }
            }
            else
            {
                INSERT_UND_INS;
            }
            break;
        }

        case OPCODE_I_TYPE_1:
        {
            uint32_t imm = (curIns >> 20);
            if (imm & (1 << 11))
            {
                imm |= 0xFFFFF000;
            }

            switch (funct3)
            {
                INSERT_REGIMM_INS(0x0, IR_OPCODE_ADDI, rd, rs1, imm);
                INSERT_REGIMM_INS(0x4, IR_OPCODE_XORI, rd, rs1, imm);
                INSERT_REGIMM_INS(0x6, IR_OPCODE_ORI, rd, rs1, imm);
                INSERT_REGIMM_INS(0x7, IR_OPCODE_ANDI, rd, rs1, imm);
                INSERT_REGIMM_INS(0x2, IR_OPCODE_SLTI, rd, rs1, imm);
                INSERT_REGIMM_INS(0x3, IR_OPCODE_SLTIU, rd, rs1, imm);
            case 0x1:
            {
                if (((curIns >> 25)) == 0)
                {
                    IRC.opcode = IR_OPCODE_SLLI;
                    IRC.rd = rd;
                    IRC.rs1 = rs1;
                    IRC.imm = imm & 0x1f;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else
                {
                    INSERT_UND_INS;
                }
                break;
            }
            case 0x5:
            {
                if (((curIns >> 25)) == 0)
                {
                    IRC.opcode = IR_OPCODE_SRLI;
                    IRC.rd = rd;
                    IRC.rs1 = rs1;
                    IRC.imm = imm & 0x1f;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else if (((curIns >> 25)) == 0x20)
                {
                    IRC.opcode = IR_OPCODE_SRAI;
                    IRC.rd = rd;
                    IRC.rs1 = rs1;
                    IRC.imm = imm & 0x1f;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else
                {
                    INSERT_UND_INS;
                }
                break;
            }
            default:
                INSERT_UND_INS;
                break;
            }
            break;
        }

        case OPCODE_I_TYPE_2:
        {
            uint32_t imm = (curIns >> 20);
            if (imm & (1 << 11))
            {
                imm |= 0xFFFFF000;
            }

            switch (funct3)
            {
                INSERT_REGIMM_INS(0x0, IR_OPCODE_LB, rd, rs1, imm);
                INSERT_REGIMM_INS(0x1, IR_OPCODE_LH, rd, rs1, imm);
                INSERT_REGIMM_INS(0x2, IR_OPCODE_LW, rd, rs1, imm);
                INSERT_REGIMM_INS(0x4, IR_OPCODE_LBU, rd, rs1, imm);
                INSERT_REGIMM_INS(0x5, IR_OPCODE_LHU, rd, rs1, imm);
            default:
                INSERT_UND_INS;
                break;
            }

            break;
        }

        case OPCODE_S_TYPE_1:
        {
            uint32_t imm = (rd) | (funct7 << 5);
            if (curIns & (1 << 31))
            {
                imm |= 0xFFFFF000;
            }
            switch (funct3)
            {
                INSERT_STORE_INS(0x0, IR_OPCODE_SB, rs1, rs2, imm);
                INSERT_STORE_INS(0x1, IR_OPCODE_SH, rs1, rs2, imm);
                INSERT_STORE_INS(0x2, IR_OPCODE_SW, rs1, rs2, imm);
            default:
                INSERT_UND_INS;
                break;
            }
            break;
        }

        case OPCODE_B_TYPE_1:
        {
            // uint32_t imm = (((curIns >> 8) & 0xF) << 1) | (((curIns >> 7) & 1) << 11) | (((curIns >> 25) & 0x3F) << 5);

            uint32_t imm_11 = (curIns >> 7) & 0x1;
            uint32_t imm_1_to_4 = (curIns >> 8) & 0xF;
            uint32_t imm_5_to_10 = (curIns >> 25) & 0x3F;
            uint32_t imm_12 = (curIns >> 31) & 0x1;
            int32_t imm = (imm_12 << 12) | (imm_11 << 11) | (imm_5_to_10 << 5) | (imm_1_to_4 << 1);
            imm <<= 19;
            imm >>= 19;

            //st = 0;
            switch (funct3)
            {
                INSERT_BRUNCH_INS(0x0, IR_OPCODE_BEQ, rs1, rs2, curVaddr + imm);
                INSERT_BRUNCH_INS(0x1, IR_OPCODE_BNE, rs1, rs2, curVaddr + imm);
                INSERT_BRUNCH_INS(0x4, IR_OPCODE_BLT, rs1, rs2, curVaddr + imm);
                INSERT_BRUNCH_INS(0x5, IR_OPCODE_BGE, rs1, rs2, curVaddr + imm);
                INSERT_BRUNCH_INS(0x6, IR_OPCODE_BLTU, rs1, rs2, curVaddr + imm);
                INSERT_BRUNCH_INS(0x7, IR_OPCODE_BGEU, rs1, rs2, curVaddr + imm);

            default:
                st = 1;
                INSERT_UND_INS;
                break;
            }

            break;
        }

        case 0b1101111: // JAL
        {
            uint32_t imm = (curIns & 0xff000) | ((curIns >> 9) & 0x800) | ((curIns >> 20) & 0x7fe) | ((curIns >> 11) & 0xff00000);
            if (imm & (1 << 20))
            {
                imm |= 0xfff00000;
            }

            IRC.opcode = IR_OPCODE_JAL;
            IRC.rs1 = curVaddr + 4;
            IRC.rd = rd;              // RD = PC + 4
            IRC.imm = curVaddr + imm; // PC = PC + imm ; jmp PC + imm
            ret = DBInsertIRCode(curDB, IRC);
            st = 0;
            break;
        }

        case 0b1100111: // JALR
        {
            if (funct3 == 0)
            {
                uint32_t imm = (curIns >> 20);
                if (curIns & (1 << 31))
                {
                    imm |= 0xFFFFF000;
                }
                IRC.opcode = IR_OPCODE_JALR;
                IRC.rd = rd;
                IRC.rs2 = curVaddr + 4; // rd = pc + 4
                IRC.rs1 = rs1;
                IRC.imm = imm; // pc = rs1 + imm
                ret = DBInsertIRCode(curDB, IRC);
                st = 0;
                break;
            }
            else
            {
                INSERT_UND_INS;
            }
            break;
        }

        case 0b0110111: // lui
        {
            uint32_t imm = (curIns >> 12);

            IRC.opcode = IR_OPCODE_LUI;
            IRC.rd = rd;
            IRC.imm = imm << 12;
            ret = DBInsertIRCode(curDB, IRC);
            break;
        }

        case 0b0010111: // auipc
        {
            uint32_t imm = (curIns >> 12);

            IRC.opcode = IR_OPCODE_AUIPC;
            IRC.rd = rd;
            IRC.imm = curVaddr + (imm << 12);
            ret = DBInsertIRCode(curDB, IRC);
            break;
        }

#if ENABLE_RV32C
        // RV32C
        // switch (opcode)
        //{
        case 0b00:
            switch (funct3)
            {
            case 0b000: // C.ADDI4SPN
            {
                uint32_t nzuimm = (curIns >> 5) & 0xFF;
                uint32_t nzuimm_3 = nzuimm & 1;
                uint32_t nzuimm_2 = (nzuimm >> 1) & 1;
                uint32_t nzuimm_6_9 = (nzuimm >> 2) & 0xF;
                uint32_t nzuimm_4_5 = (nzuimm >> 6) & 0b11;

                nzuimm = (nzuimm_2 << 2) | (nzuimm_3 << 3) | (nzuimm_4_5 << 4) | (nzuimm_6_9 << 6);
                if (!nzuimm)
                {
                    INSERT_UND_INS;
                }
                uint32_t rd_c = (curIns >> 2) & 0b111;
                rd = rd_c + 8;
                IRC.opcode = IR_OPCODE_ADDI;
                IRC.rd = rd;
                IRC.rs1 = 2;
                IRC.imm = nzuimm;
                ret = DBInsertIRCode(curDB, IRC);

                break;
            }

            case 0b010: // C.LW rd, offset(rs1)
            {
                uint32_t uimm_3_5 = (curIns >> 10) & 0b111;
                uint32_t uimm_2 = (curIns >> 6) & 1;
                uint32_t uimm_6 = (curIns >> 5) & 1;
                uint32_t uimm = (uimm_2 << 2) | (uimm_6 << 6) | (uimm_3_5 << 3);

                uint32_t rd_c = (curIns >> 2) & 0b111;
                uint32_t rs1_c = (curIns >> 7) & 0b111;

                IRC.opcode = IR_OPCODE_LW;
                IRC.rd = rd_c + 8;
                IRC.rs1 = rs1_c + 8;
                IRC.imm = uimm;
                ret = DBInsertIRCode(curDB, IRC);
                break;
            }
            case 0b110: // C.SW rs2, offset(rs1)
            {
                uint32_t uimm_3_5 = (curIns >> 10) & 0b111;
                uint32_t uimm_2 = (curIns >> 6) & 1;
                uint32_t uimm_6 = (curIns >> 5) & 1;
                uint32_t uimm = (uimm_2 << 2) | (uimm_6 << 6) | (uimm_3_5 << 3);

                uint32_t rd_c = (curIns >> 2) & 0b111;
                uint32_t rs1_c = (curIns >> 7) & 0b111;

                IRC.opcode = IR_OPCODE_SW;
                IRC.rs2 = rd_c + 8;
                IRC.rs1 = rs1_c + 8;
                IRC.imm = uimm;
                ret = DBInsertIRCode(curDB, IRC);
                break;
            }
            default:
                INSERT_UND_INS;
                break;
            }
            break;

        case 0b01:
            switch (funct3)
            {
            case 0b000: // C.ADDI rd, rs1, imm
            {
                uint32_t nzsimm_5 = (curIns >> 12) & 1;
                uint32_t nzsimm = ((curIns >> 2) & 0x1F) | (nzsimm_5 << 5);
                uint32_t rd = (curIns >> 7) & 0x1F;
                if (nzsimm_5)
                {
                    nzsimm |= ~((1 << 5) - 1);
                }

                if (!rd)
                {
                    // INSERT_UND_INS;
                    if (nzsimm)
                    {
                        INSERT_UND_INS;
                    }
                }

                IRC.opcode = IR_OPCODE_ADDI;
                IRC.rd = rd;
                IRC.rs1 = rd;
                IRC.imm = nzsimm;
                ret = DBInsertIRCode(curDB, IRC);
                break;
            }

            case 0b001: // C.JAL rd, offset
            {
                uint32_t simm_field = (curIns >> 2);

                uint32_t simm_5 = (simm_field & 1);
                uint32_t simm_1_3 = ((simm_field >> 1) & 0b111);
                uint32_t simm_7 = ((simm_field >> 4) & 0b1);
                uint32_t simm_6 = ((simm_field >> 5) & 0b1);
                uint32_t simm_10 = ((simm_field >> 6) & 0b1);
                uint32_t simm_8_9 = ((simm_field >> 7) & 0b11);
                uint32_t simm_4 = ((simm_field >> 9) & 0b1);
                uint32_t simm_11 = ((simm_field >> 10) & 0b1);

                uint32_t simm = (simm_1_3 << 1) | (simm_4 << 4) | (simm_5 << 5) | (simm_6 << 6) |
                                (simm_7 << 7) | (simm_8_9 << 8) | (simm_10 << 10) | (simm_11 << 11);
                if (simm_11)
                {
                    simm |= ~((1 << 11) - 1);
                }

                IRC.opcode = IR_OPCODE_JAL;
                IRC.rs1 = curVaddr + 2;
                IRC.rd = 1;                // RD = PC + 2; RD = x1
                IRC.imm = curVaddr + simm; // PC = PC + imm ; jmp PC + imm
                ret = DBInsertIRCode(curDB, IRC);

                break;
            }
            case 0b010: // C.LI rd, rs1, imm
            {
                uint32_t simm_5 = (curIns >> 12) & 1;
                uint32_t simm = ((curIns >> 2) & 0x1F) | (simm_5 << 5);
                if (simm_5)
                {
                    simm |= ~((1 << 5) - 1);
                }
                uint32_t rd = (curIns >> 7) & 0x1F;

                IRC.opcode = IR_OPCODE_ADDI;
                IRC.rd = rd;
                IRC.rs1 = 0;
                IRC.imm = simm;
                ret = DBInsertIRCode(curDB, IRC);

                break;
            }

            case 0b011: // C.ADDI16SP rd, rs1, imm / C.LUI rd, imm
            {
                uint32_t rd = (curIns >> 7) & 0x1F;
                if (rd == 2)
                {
                    uint32_t nzsimm_5 = (curIns >> 2) & 1;
                    uint32_t nzsimm_7_8 = ((curIns >> 2) >> 1) & 0b11;
                    uint32_t nzsimm_6 = ((curIns >> 2) >> 3) & 0b1;
                    uint32_t nzsimm_4 = ((curIns >> 2) >> 4) & 0b1;
                    uint32_t nzsimm_9 = (curIns >> 12) & 0b1;

                    uint32_t nzsimm = (nzsimm_4 << 4) | (nzsimm_5 << 5) | (nzsimm_6 << 6) |
                                      (nzsimm_7_8 << 7) | (nzsimm_9 << 9);
                    if (!nzsimm)
                    {
                        INSERT_UND_INS;
                    }
                    if (nzsimm_9)
                    {
                        nzsimm |= ~((1 << 9) - 1);
                    }
                    IRC.opcode = IR_OPCODE_ADDI;
                    IRC.rd = 2;
                    IRC.rs1 = 2;
                    IRC.imm = (int32_t)nzsimm;
                    ret = DBInsertIRCode(curDB, IRC);
                    break;
                }
                else // C.LUI rd, imm
                {
                    uint32_t imm_12_16 = (curIns >> 2) & 0x1F;
                    uint32_t imm_17 = (curIns >> 12) & 0x1;
                    uint32_t imm = (imm_17 << 17) | (imm_12_16 << 12);
                    if (imm_17)
                    {
                        imm |= ~((1 << 17) - 1);
                    }
                    IRC.opcode = IR_OPCODE_LUI;
                    IRC.rd = rd;
                    IRC.imm = imm;
                    ret = DBInsertIRCode(curDB, IRC);
                    break;
                }

                break;
            }

            case 0b100:
            {
                uint32_t op1 = (curIns >> 10) & 0b111;
                uint32_t rd_c = (curIns >> 7) & 0b111;
                if ((op1 & 0b011) == 0b010)
                { // C.ANDI rd, rs1, imm
                    uint32_t nzsimm_5 = (curIns >> 12) & 1;
                    uint32_t nzsimm = ((curIns >> 2) & 0x1F) | (nzsimm_5 << 5);
                    if (!nzsimm)
                    {
                        INSERT_UND_INS;
                    }
                    if (nzsimm_5)
                    {
                        nzsimm |= ~((1 << 5) - 1);
                    }

                    IRC.opcode = IR_OPCODE_ANDI;
                    IRC.rd = rd_c + 8;
                    IRC.rs1 = rd_c + 8;
                    IRC.imm = nzsimm;
                    ret = DBInsertIRCode(curDB, IRC);
                    break;
                }
                else
                {
                    switch (op1)
                    {
                    case 0b000: // C.SRLI rd, rs1, imm
                    {
                        uint32_t nzuimm = ((curIns >> 2) & 0x1F);
                        if (!nzuimm)
                        {
                            INSERT_UND_INS;
                        }
                        IRC.opcode = IR_OPCODE_SRLI;
                        IRC.rd = rd_c + 8;
                        IRC.rs1 = rd_c + 8;
                        IRC.imm = nzuimm;
                        ret = DBInsertIRCode(curDB, IRC);
                        break;
                    }
                    case 0b001: // C.SRAI rd, rs1, imm
                    {
                        uint32_t nzuimm = ((curIns >> 2) & 0x1F);
                        if (!nzuimm)
                        {
                            INSERT_UND_INS;
                        }
                        IRC.opcode = IR_OPCODE_SRAI;
                        IRC.rd = rd_c + 8;
                        IRC.rs1 = rd_c + 8;
                        IRC.imm = nzuimm;
                        ret = DBInsertIRCode(curDB, IRC);
                        break;
                    }

                    case 0b011:
                    {
                        uint32_t rs2_c = (curIns >> 2) & 0b111;
                        uint32_t op2 = (curIns >> 5) & 0b11;
                        switch (op2)
                        {
                        case 0b00: // C.SUB rd, rs1, rs2
                        {
                            IRC.opcode = IR_OPCODE_SUB;
                            IRC.rd = rd_c + 8;
                            IRC.rs1 = rd_c + 8;
                            IRC.rs2 = rs2_c + 8;
                            ret = DBInsertIRCode(curDB, IRC);
                            break;
                        }
                        case 0b01: // C.XOR rd, rs1, rs2
                        {
                            IRC.opcode = IR_OPCODE_XOR;
                            IRC.rd = rd_c + 8;
                            IRC.rs1 = rd_c + 8;
                            IRC.rs2 = rs2_c + 8;
                            ret = DBInsertIRCode(curDB, IRC);
                            break;
                        }
                        case 0b10: // C.OR rd, rs1, rs2
                        {
                            IRC.opcode = IR_OPCODE_OR;
                            IRC.rd = rd_c + 8;
                            IRC.rs1 = rd_c + 8;
                            IRC.rs2 = rs2_c + 8;
                            ret = DBInsertIRCode(curDB, IRC);
                            break;
                        }
                        case 0b11: // C.AND rd, rs1, rs2
                        {
                            IRC.opcode = IR_OPCODE_AND;
                            IRC.rd = rd_c + 8;
                            IRC.rs1 = rd_c + 8;
                            IRC.rs2 = rs2_c + 8;
                            ret = DBInsertIRCode(curDB, IRC);
                            break;
                        }
                        default:
                            INSERT_UND_INS;
                            break;
                        }
                        break;
                    }

                    default:
                        INSERT_UND_INS;
                        break;
                    }
                }

                break;
            }

            case 0b101: // C.J rd, offset
            {
                uint32_t simm_field = (curIns >> 2);

                uint32_t simm_5 = (simm_field & 1);
                uint32_t simm_1_3 = ((simm_field >> 1) & 0b111);
                uint32_t simm_7 = ((simm_field >> 4) & 0b1);
                uint32_t simm_6 = ((simm_field >> 5) & 0b1);
                uint32_t simm_10 = ((simm_field >> 6) & 0b1);
                uint32_t simm_8_9 = ((simm_field >> 7) & 0b11);
                uint32_t simm_4 = ((simm_field >> 9) & 0b1);
                uint32_t simm_11 = ((simm_field >> 10) & 0b1);

                uint32_t simm = (simm_1_3 << 1) | (simm_4 << 4) | (simm_5 << 5) | (simm_6 << 6) |
                                (simm_7 << 7) | (simm_8_9 << 8) | (simm_10 << 10) | (simm_11 << 11);
                if (simm_11)
                {
                    simm |= ~((1 << 11) - 1);
                }

                IRC.opcode = IR_OPCODE_JAL;
                IRC.rs1 = curVaddr + 2;
                IRC.rd = 0;                // RD = PC + 2; RD = x0
                IRC.imm = curVaddr + simm; // PC = PC + imm ; jmp PC + imm
                ret = DBInsertIRCode(curDB, IRC);
                break;
            }

            case 0b110: // C.BEQZ rs1, rs2, offset
            {
                uint32_t simm_5 = (curIns >> 2) & 1;
                uint32_t simm_1_2 = ((curIns >> 3)) & 0b11;
                uint32_t simm_6_7 = ((curIns >> 5)) & 0b11;
                uint32_t simm_3_4 = ((curIns >> 10)) & 0b11;
                uint32_t simm_8 = ((curIns >> 12)) & 0b1;
                uint32_t simm = (simm_1_2 << 1) | (simm_3_4 << 3) | (simm_5 << 5) | (simm_6_7 << 6) | (simm_8 << 8);
                if (simm_8)
                {
                    simm |= ~((1 << 8) - 1);
                }
                uint32_t rd_c = (curIns >> 7) & 0b111;

                IRC.opcode = IR_OPCODE_BEQ;
                IRC.rs1 = rd_c + 8;
                IRC.rs2 = 0;
                IRC.imm = curVaddr + simm;
                ret = DBInsertIRCode(curDB, IRC);
                //st = 0;
                break;
            }

            case 0b111: // C.BNEZ rs1, rs2, offset
            {
                uint32_t simm_5 = (curIns >> 2) & 1;
                uint32_t simm_1_2 = ((curIns >> 3)) & 0b11;
                uint32_t simm_6_7 = ((curIns >> 5)) & 0b11;
                uint32_t simm_3_4 = ((curIns >> 10)) & 0b11;
                uint32_t simm_8 = ((curIns >> 12)) & 0b1;
                uint32_t simm = (simm_1_2 << 1) | (simm_3_4 << 3) | (simm_5 << 5) | (simm_6_7 << 6) | (simm_8 << 8);
                if (simm_8)
                {
                    simm |= ~((1 << 8) - 1);
                }
                uint32_t rd_c = (curIns >> 7) & 0b111;

                IRC.opcode = IR_OPCODE_BNE;
                IRC.rs1 = rd_c + 8;
                IRC.rs2 = 0;
                IRC.imm = curVaddr + simm;
                ret = DBInsertIRCode(curDB, IRC);
                //st = 0;
                break;
            }

            default:
                INSERT_UND_INS;
                break;
            }
            break;

        case 0b10:
            rd = (curIns >> 7) & 0x1F;
            switch (funct3)
            {
            case 0b000: // C.SLLI rd, rs1, imm
            {
                uint32_t nzuimm = (curIns >> 2) & 0x1F;
                if (!nzuimm)
                {
                    INSERT_UND_INS;
                }
                IRC.opcode = IR_OPCODE_SLLI;
                IRC.rd = rd;
                IRC.rs1 = rd;
                IRC.imm = nzuimm;
                ret = DBInsertIRCode(curDB, IRC);
                break;
            }

            case 0b010: // C.LWSP rd, offset(rs1)
            {
                uint32_t uimm_6_7 = (curIns >> 2) & 0b11;
                uint32_t uimm_2_4 = ((curIns >> 2) >> 2) & 0b111;
                uint32_t uimm_5 = (curIns >> 12) & 1;
                uint32_t uimm = (uimm_2_4 << 2) | (uimm_5 << 5) | (uimm_6_7 << 6);

                IRC.opcode = IR_OPCODE_LW;
                IRC.rd = rd;
                IRC.rs1 = 2; // sp
                IRC.imm = uimm;
                ret = DBInsertIRCode(curDB, IRC);
                break;
            }

            case 0b100:
            {
                uint32_t rd_cc = (curIns >> 12) & 1;
                uint32_t rs1 = (curIns >> 7) & 0x1F;
                uint32_t rs2 = (curIns >> 2) & 0x1F;
                if (rd_cc && (rs1 > 0) && (rs1 < 32) && (rs2 == 0)) // C.JALR
                {
                    IRC.opcode = IR_OPCODE_JALR;
                    IRC.rd = 1;
                    IRC.rs2 = curVaddr + 2; // rd = pc + 2
                    IRC.rs1 = rs1;
                    IRC.imm = 0; // pc = rs1
                    ret = DBInsertIRCode(curDB, IRC);
                    st = 0;
                    break;
                }
                else if (rd_cc && (rs1 != 0) && (rs2 != 0)) // C.ADD
                {

                    IRC.opcode = IR_OPCODE_ADD;
                    IRC.rd = rs1;
                    IRC.rs2 = rs2;
                    IRC.rs1 = rs1;
                    ret = DBInsertIRCode(curDB, IRC);
                    break;
                }
                else if ((!rd_cc) && (rs1 > 0) && (rs1 < 32) && (rs2 == 0)) // C.JR  /jalr x0, 0(rs1).
                {
                    IRC.opcode = IR_OPCODE_JALR;
                    IRC.rd = 0;
                    IRC.rs2 = curVaddr + 2; // rd = pc + 2 //rd = x0
                    IRC.rs1 = rs1;
                    IRC.imm = 0; // pc = rs1
                    ret = DBInsertIRCode(curDB, IRC);
                    st = 0;
                    break;
                }
                else if ((!rd_cc) && (rs1 != 0) && (rs2 != 0)) // C.MV rd, rs1, rs2
                {
                    IRC.opcode = IR_OPCODE_ADD;
                    IRC.rd = rs1;
                    IRC.rs2 = rs2;
                    IRC.rs1 = 0;
                    ret = DBInsertIRCode(curDB, IRC);
                    break;
                }
                else if (rd_cc && (!rs1) && (!rs2)) // C.EBREAK
                {

                    INSERT_UND_INS;
                }
                else
                {
                    INSERT_UND_INS;
                }

                break;
            }
            case 0b110:
            {
                uint32_t uimm_6_7 = (curIns >> 7) & 0b11;
                uint32_t uimm_2_5 = (curIns >> 9) & 0b1111;
                uint32_t uimm = (uimm_2_5 << 2) | (uimm_6_7 << 6);
                uint32_t rs2 = (curIns >> 2) & 0x1F;

                IRC.opcode = IR_OPCODE_SW;
                IRC.rs1 = 2; // [sp + imm] = rs2
                IRC.rs2 = rs2;
                IRC.imm = uimm;
                ret = DBInsertIRCode(curDB, IRC);
                break;
            }

                //    default:
                //        INSERT_UND_INS;
                //        break;
                //    }

            default:
                INSERT_UND_INS;
                break;
            }
            break;
            // ============ RV32C END ============
#endif

        case 0b0001111: // fence
        {
            IRC.opcode = IR_OPCODE_ADD;
            IRC.rd = 0;
            IRC.rs1 = 0;
            IRC.rs2 = 0;
            ret = DBInsertIRCode(curDB, IRC); // NOP
            // return -1;
            // printf("Fence\n");
            break;
        }

        case OPCODE_I_ZICSR:
        {
            uint32_t csr = curIns >> 20;
            IRC.imm = csr;
            IRC.rs1 = rs1;
            IRC.rd = rd;
            switch (funct3)
            {
            case 0b001: // csrrw
                IRC.opcode = IR_OPCODE_CSRRW;
                ret = DBInsertIRCode(curDB, IRC);
                break;

            case 0b010: // csrrs
                IRC.opcode = IR_OPCODE_CSRRS;
                ret = DBInsertIRCode(curDB, IRC);

                break;

            case 0b011: // csrrc
                IRC.opcode = IR_OPCODE_CSRRC;
                ret = DBInsertIRCode(curDB, IRC);

                break;

            case 0b101: // csrrwi
                IRC.opcode = IR_OPCODE_CSRRWI;
                ret = DBInsertIRCode(curDB, IRC);

                break;

            case 0b110: // cssrrsi
                IRC.opcode = IR_OPCODE_CSRRSI;
                ret = DBInsertIRCode(curDB, IRC);
                break;

            case 0b111: // csrrci
                IRC.opcode = IR_OPCODE_CSRRCI;
                ret = DBInsertIRCode(curDB, IRC);
                break;

            default:
                if ((csr == ((0b0000000 << 5) | (0b00000))) && (rs1 == 0b00000) && (funct3 == 0b000) && (rd == 0b00000))
                {
                    IRC.opcode = IR_OPCODE_ECALL;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else if ((csr == ((0b0000000 << 5) | (0b00001))) && (rs1 == 0b00000) && (funct3 == 0b000) && (rd == 0b00000))
                {
                    IRC.opcode = IR_OPCODE_EBREAK;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else if ((csr == ((0b0000000 << 5) | (0b00010))) && (rs1 == 0b00000) && (funct3 == 0b000) && (rd == 0b00000))
                {
                    IRC.opcode = IR_OPCODE_URET;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else if ((csr == ((0b0001000 << 5) | (0b00010))) && (rs1 == 0b00000) && (funct3 == 0b000) && (rd == 0b00000))
                {
                    IRC.opcode = IR_OPCODE_SRET;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else if ((csr == ((0b0010000 << 5) | (0b00010))) && (rs1 == 0b00000) && (funct3 == 0b000) && (rd == 0b00000))
                {
                    IRC.opcode = IR_OPCODE_HRET;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else if ((csr == ((0b0011000 << 5) | (0b00010))) && (rs1 == 0b00000) && (funct3 == 0b000) && (rd == 0b00000))
                {
                    IRC.opcode = IR_OPCODE_MRET;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else if ((csr == ((0b0111001 << 5) | (0b10010))) && (rs1 == 0b00000) && (funct3 == 0b000) && (rd == 0b00000))
                {
                    IRC.opcode = IR_OPCODE_DRET;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else if ((csr == ((0b0001000 << 5) | (0b00100))) && (funct3 == 0b000) && (rd == 0b00000))
                {
                    IRC.rs1 = rs1;
                    IRC.opcode = IR_OPCODE_SFENCE_VM;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else if (((csr >> 5) == (0b0001001)) && (funct3 == 0b000) && (rd == 0b00000))
                {
                    IRC.rs1 = rs1;
                    rs2 = (csr & 0x1F);
                    IRC.rs1 = rs2;
                    IRC.opcode = IR_OPCODE_SFENCE_VMA;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else if ((csr == ((0b0001000 << 5) | (0b00101))) && (rs1 == 0b00000) && (funct3 == 0b000) && (rd == 0b00000))
                {
                    IRC.opcode = IR_OPCODE_WFI;
                    ret = DBInsertIRCode(curDB, IRC);
                }
                else
                {
                    INSERT_UND_INS;
                }
                break;
            }
            break;
        }

        case OPCODE_R_ATOMIC:
        {
            // printf("ATOMIC\n");
#define ATOMIC_INS(f5_, OPC_, rd_, rs1_, rs2_, aqrl_) \
    case f5_:                                         \
    {                                                 \
        IRC.opcode = OPC_;                            \
        IRC.rs1 = rs1_;                               \
        IRC.rs2 = rs2_;                               \
        IRC.rd = rd_;                                 \
        IRC.imm = aqrl_;                              \
        ret = DBInsertIRCode(curDB, IRC);             \
        break;                                        \
    }

            uint32_t funct5 = curIns >> 27;
            if (funct3 == 0x2)
            {
                switch (funct5)
                {
                    ATOMIC_INS(0x02, IR_OPCODE_LR_W, rd, rs1, rs2, (curIns >> 25) & 0b11);
                    ATOMIC_INS(0x03, IR_OPCODE_SC_W, rd, rs1, rs2, (curIns >> 25) & 0b11);
                    ATOMIC_INS(0x01, IR_OPCODE_AMOSWAP_W, rd, rs1, rs2, (curIns >> 25) & 0b11);
                    ATOMIC_INS(0x00, IR_OPCODE_AMOADD_W, rd, rs1, rs2, (curIns >> 25) & 0b11);
                    ATOMIC_INS(0x0C, IR_OPCODE_AMOAND_W, rd, rs1, rs2, (curIns >> 25) & 0b11);
                    ATOMIC_INS(0x08, IR_OPCODE_AMOOR_W, rd, rs1, rs2, (curIns >> 25) & 0b11);
                    ATOMIC_INS(0x04, IR_OPCODE_AMOXOR_W, rd, rs1, rs2, (curIns >> 25) & 0b11);
                    ATOMIC_INS(0x14, IR_OPCODE_AMOMAX_W, rd, rs1, rs2, (curIns >> 25) & 0b11);
                    ATOMIC_INS(0x10, IR_OPCODE_AMOMIN_W, rd, rs1, rs2, (curIns >> 25) & 0b11);
                default:
                    INSERT_UND_INS;
                    break;
                }
            }
            else
            {
                INSERT_UND_INS;
            }
            break;
        }

        default:
            INSERT_UND_INS;
            break;
        }

        // dump_IRCode(IRC);

        if (ret == 1)
        {
            st = 0;
            return 0;
        }
        else if (ret == -1)
        {
            printf("Decode ERROR!\n");
            return -1;
        }
        else
        {
            // printf("ret:%d\n",ret);
        }

        curDB->src_codes_length += (compress) ? 2 : 4;
        curVaddr += (compress) ? 2 : 4;
    }

    return 0;
}
