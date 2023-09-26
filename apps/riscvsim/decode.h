#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_DECODE_BLOCKS ((1 << 5) - 1)
#define INS_PER_DB (16)

#define ENABLE_RV32C (1)

#define OPCODE_R_TYPE_1 0b0110011
#define OPCODE_I_TYPE_1 0b0010011
#define OPCODE_I_TYPE_2 0b0000011
#define OPCODE_S_TYPE_1 0b0100011
#define OPCODE_B_TYPE_1 0b1100011
#define OPCODE_I_ZICSR 0b1110011
#define OPCODE_R_ATOMIC 0b0101111

typedef enum IROpcode
{
    IR_OPCODE_EMPTY = 0,
    IR_OPCODE_UND,
    IR_OPCODE_NOP,
    IR_OPCODE_SET,
    IR_OPCODE_CPY,
    IR_OPCODE_JMP,

    IR_OPCODE_ADD,
    IR_OPCODE_SUB,
    IR_OPCODE_XOR,
    IR_OPCODE_OR,
    IR_OPCODE_AND,
    IR_OPCODE_SLL,
    IR_OPCODE_SRL,
    IR_OPCODE_SRA,
    IR_OPCODE_SLT,
    IR_OPCODE_SLTU,

    IR_OPCODE_ADDI,
    IR_OPCODE_XORI,
    IR_OPCODE_ORI,
    IR_OPCODE_ANDI,
    IR_OPCODE_SLLI,
    IR_OPCODE_SRLI,
    IR_OPCODE_SRAI,
    IR_OPCODE_SLTI,
    IR_OPCODE_SLTIU,

    IR_OPCODE_LB,
    IR_OPCODE_LH,
    IR_OPCODE_LW,
    IR_OPCODE_LBU,
    IR_OPCODE_LHU,

    IR_OPCODE_SB,
    IR_OPCODE_SH,
    IR_OPCODE_SW,

    IR_OPCODE_BEQ,
    IR_OPCODE_BNE,
    IR_OPCODE_BLT,
    IR_OPCODE_BGE,
    IR_OPCODE_BLTU,
    IR_OPCODE_BGEU,

    IR_OPCODE_JAL,
    IR_OPCODE_JALR,
    IR_OPCODE_LUI,
    IR_OPCODE_AUIPC,

    IR_OPCODE_MUL,
    IR_OPCODE_MULH,
    IR_OPCODE_MULHSU,
    IR_OPCODE_MULHU,
    IR_OPCODE_DIV,
    IR_OPCODE_DIVU,
    IR_OPCODE_REM,
    IR_OPCODE_REMU,

    IR_OPCODE_CSRRW,
    IR_OPCODE_CSRRS,
    IR_OPCODE_CSRRC,
    IR_OPCODE_CSRRWI,
    IR_OPCODE_CSRRSI,
    IR_OPCODE_CSRRCI,

    IR_OPCODE_ECALL,
    IR_OPCODE_EBREAK,
    IR_OPCODE_URET,
    IR_OPCODE_SRET,
    IR_OPCODE_HRET,
    IR_OPCODE_MRET,
    IR_OPCODE_DRET,
    IR_OPCODE_SFENCE_VM,
    IR_OPCODE_SFENCE_VMA,
    IR_OPCODE_WFI,

    IR_OPCODE_LR_W,
    IR_OPCODE_SC_W,
    IR_OPCODE_AMOSWAP_W,
    IR_OPCODE_AMOADD_W,
    IR_OPCODE_AMOAND_W,
    IR_OPCODE_AMOOR_W,
    IR_OPCODE_AMOXOR_W,
    IR_OPCODE_AMOMAX_W,
    IR_OPCODE_AMOMIN_W,

} IROpcode;

typedef struct IRCode
{
    // struct IRCode *next; src_len
    uint32_t rd, rs1, rs2;
    uint32_t imm;
    IROpcode opcode;
} IRCode;

typedef struct DecodeBlock
{
    uint32_t src_pc;
    uint32_t src_codes_length;
    uint32_t IR_Length;
    uint32_t exec_cnt;
    uint32_t src_ins_len;
    IRCode Ins[INS_PER_DB];
} DecodeBlock;

int GetCachedDBByAddr(uint32_t src_pc, DecodeBlock **DB, uint32_t *offset);

int DecodeSrc(uint32_t VirtAddr);
int DecodeInit();

void dump_IRCode(IRCode INS);
void dump_DB(uint32_t src_pc);
void dump_all_DB();
