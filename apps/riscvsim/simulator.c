#include "simulator.h"
#include "mem.h"
#include "decode.h"

//#include <pthread.h>
//#include <time.h>
//#include <unistd.h>
//#if defined(_WIN64) || defined(_WIN32)
//#include <conio.h>
//#endif

#include "jit-gen-arm.h"
#include "llapi.h"




uint32_t REG[32];
uint32_t CurPC = 0;
uint32_t Instructions = 0;

#ifndef __max
#define __max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef __min
#define __min(a, b) (((a) < (b)) ? (a) : (b))
#endif

uint32_t REG_CSR_MSCRATCH = 0;
uint32_t REG_CSR_MTVEC = 0;
uint32_t REG_CSR_MIP = 0;
uint32_t REG_CSR_MIE = 0;
uint32_t REG_CSR_PMPCFG[16] = {0};
uint32_t REG_CSR_PMPADDR[64] = {0};

static unsigned long ctz(unsigned long x)
{
    unsigned long ret = 0;
    if (x == 0)
        return 8 * sizeof(x);
    while (!(x & 1UL))
    {
        ret++;
        x = x >> 1;
    }
    return ret;
}



uint32_t timerVal = 0;
uint32_t stdout_port = 0;

void update_perp()
{
    timerVal = llapi_get_tick_ms();
}




int pmp_decode(uint32_t n, uint32_t *prot_out, uint32_t *addr_out, uint32_t *log2len)
{
    if (n >= PMP_COUNT || !prot_out || !addr_out || !log2len)
        return -1;
    uint32_t pmpcfg_csr = CSR_PMPCFG0 + (n >> 2);
    uint32_t pmpcfg_shift = (n & 3) << 3;
    uint32_t pmpaddr_csr = CSR_PMPADDR0 + n;

    uint32_t cfgmask = (0xffUL << pmpcfg_shift);
    uint32_t pmpcfg = /*csr_read_num(pmpcfg_csr)*/ REG_CSR_PMPCFG[pmpcfg_csr - CSR_PMPCFG0] & cfgmask;
    uint32_t prot = pmpcfg >> pmpcfg_shift;

    uint32_t addr;
    uint32_t len;
    uint32_t t1;
    if ((prot & PMP_A) == PMP_A_NAPOT)
    {
        addr = REG_CSR_PMPADDR[pmpaddr_csr - CSR_PMPADDR0];
        if (addr == -1UL)
        {
            addr = 0;
            len = 32;
        }
        else
        {
            t1 = ctz(~addr);
            addr = (addr & ~((1UL << t1) - 1)) << PMP_SHIFT;
            len = (t1 + PMP_SHIFT + 1);
        }
    }
    else
    {
        addr = REG_CSR_PMPADDR[pmpaddr_csr - CSR_PMPADDR0] << PMP_SHIFT;
        len = PMP_SHIFT;
    }

    *prot_out = prot;
    *addr_out = addr;
    *log2len = len;

    return 0;
}

int CSR_Read(uint32_t address, uint32_t *val)
{
    switch (address)
    {
    case CSR_MSCRATCH:
        *val = REG_CSR_MSCRATCH;
        return 0;
    case CSR_MTVEC:
        *val = REG_CSR_MTVEC;
        return 0;
    case CSR_MHARTID:
        *val = 0;
        return 0;
    case CSR_MISA:
#define ADD_ISA(v, x) ((v) |= (1 << ((x) - 'A')));
        //*val &= (~0x3FFFFFFF);
        *val = 0;
        ADD_ISA(*val, 'A');
        ADD_ISA(*val, 'I');
        ADD_ISA(*val, 'C');
        ADD_ISA(*val, 'M');
        ADD_ISA(*val, 'S');
        ADD_ISA(*val, 'U');
        *val |= (1 << 30);
        return 0;
    case CSR_MIE:
        *val = REG_CSR_MIE;
        return 0;
    case CSR_MIP:
        *val = REG_CSR_MIP;
        return 0;

    case CSR_PMPADDR0 ... CSR_PMPADDR63:
    {
        uint32_t n = address - CSR_PMPADDR0;
        *val = REG_CSR_PMPADDR[n];

        return 0;
    }

    case CSR_PMPCFG0 ... CSR_PMPCFG15:
    {
        uint32_t n = address - CSR_PMPCFG0;
        *val = REG_CSR_PMPCFG[n];

        return 0;
    }

    default:
        *val = 0;
        break;
    }

    printf("CSR RD:%08x at %08x\n", address, CurPC);
    return -1;
}

int CSR_Write(uint32_t address, uint32_t val)
{
    switch (address)
    {
    case CSR_MSCRATCH:
        REG_CSR_MSCRATCH = val;
        return 0;
    case CSR_MTVEC:
        REG_CSR_MTVEC = val;
        return 0;
    case CSR_MIE:
        REG_CSR_MIE = val;
        return 0;

    case CSR_PMPADDR0 ... CSR_PMPADDR63:
    {
        uint32_t n = address - CSR_PMPADDR0;
        REG_CSR_PMPADDR[n] = val;

        return 0;
    }

    case CSR_PMPCFG0 ... CSR_PMPCFG15:
    {
        uint32_t n = address - CSR_PMPCFG0;
        REG_CSR_PMPCFG[n] = val;
        {
            uint32_t x = n * 4;
            for (int i = x; i < x + 4; i++)
            {
                uint32_t prot, addr, log2len;
                pmp_decode(i, &prot, &addr, &log2len);
                //printf("PMPSET:%d, %02x,%08x,%d\n", x + i, prot, addr, log2len);
                printf("PMPSET:%d, %02x,%08x~%08x\n", x + i, prot, addr, addr + (1 << log2len) - 1);
            }
        }
        return 0;
    }

    case CSR_MIP:
    case CSR_MISA:
    case CSR_MHARTID:
        return 0;

    default:
        printf("CSR WR:%08x = %08x at %08x\n", address, val, CurPC);
        break;
    }

    return -1;
}


#define EXEC_CONTIUNE   0
#define EXEC_JUMP_OUT   2

#define EXEC_REG_OPC(OPC, rd, rs1, rs2, s) \
    case OPC:                              \
        REG[rd] = REG[rs1] s REG[rs2];     \
        break;                             \

#define EXEC_REGIMM_OPC(OPC, rd, rs1, imm, s) \
    case OPC:                                 \
        REG[rd] = REG[rs1] s imm;             \
        break;                                \

#define EXEC_BRUNCH(OPC, rs1, rs2, s)                      \
    case OPC:                                              \
        if ((REG[rs1] s REG[rs2]))                         \
        {                                                  \
            *next_pc = INS->imm;                           \
            return EXEC_JUMP_OUT;                          \
        }                                                  \
            break;


static int inline ExecOneLine(IRCode *INS, uint32_t *next_pc)
{
        REG[0] = 0;
        switch (INS->opcode)
        {
            EXEC_REG_OPC(IR_OPCODE_ADD, INS->rd, INS->rs1, INS->rs2, +);
            EXEC_REG_OPC(IR_OPCODE_SUB, INS->rd, INS->rs1, INS->rs2, -);
            EXEC_REG_OPC(IR_OPCODE_XOR, INS->rd, INS->rs1, INS->rs2, ^);
            EXEC_REG_OPC(IR_OPCODE_OR, INS->rd, INS->rs1, INS->rs2, |);
            EXEC_REG_OPC(IR_OPCODE_AND, INS->rd, INS->rs1, INS->rs2, &);
            EXEC_REG_OPC(IR_OPCODE_SLL, INS->rd, INS->rs1, INS->rs2, <<);
            EXEC_REG_OPC(IR_OPCODE_SRL, INS->rd, INS->rs1, INS->rs2, >>);

        case IR_OPCODE_SRA: 
            REG[INS->rd] = ((int32_t)REG[INS->rs1]) >> REG[INS->rs2];
            break; 

        case IR_OPCODE_SLT: 
            REG[INS->rd] = (((int32_t)REG[INS->rs1]) < ((int32_t)REG[INS->rs2]) ? 1 : 0);
            break; 
        case IR_OPCODE_SLTU: 
            REG[INS->rd] = ((REG[INS->rs1]) < (REG[INS->rs2]) ? 1 : 0);
            break; 
            //================= RV32M

        case IR_OPCODE_MUL: 
            REG[INS->rd] = REG[INS->rs1] * REG[INS->rs2];
            break; 
        case IR_OPCODE_MULH:
        {
            int64_t a = REG[INS->rs1], b = REG[INS->rs2];
            REG[INS->rd] = (a * b) >> 32;
            break;
        }
        case IR_OPCODE_MULHSU:
        {
            int64_t a = REG[INS->rs1];
            uint64_t b = REG[INS->rs2];
            REG[INS->rd] = (a * b) >> 32;
            break;
        }
        case IR_OPCODE_MULHU:
        {
            uint64_t a = REG[INS->rs1];
            uint64_t b = REG[INS->rs2];
            REG[INS->rd] = (a * b) >> 32;
            break;
        }
        case IR_OPCODE_DIV: 
            REG[INS->rd] = REG[INS->rs1] / (int32_t)REG[INS->rs2];
            break; 
        case IR_OPCODE_DIVU: 
            REG[INS->rd] = REG[INS->rs1] / REG[INS->rs2];
            break; 
        case IR_OPCODE_REM: 
            REG[INS->rd] = REG[INS->rs1] % (int32_t)REG[INS->rs2];
            break; 
        case IR_OPCODE_REMU: 
            REG[INS->rd] = REG[INS->rs1] % REG[INS->rs2];
            break; 
            //============================ RV32M ===========================

            //=================
            EXEC_REGIMM_OPC(IR_OPCODE_ADDI, INS->rd, INS->rs1, INS->imm, +);
            EXEC_REGIMM_OPC(IR_OPCODE_XORI, INS->rd, INS->rs1, INS->imm, ^);
            EXEC_REGIMM_OPC(IR_OPCODE_ORI, INS->rd, INS->rs1, INS->imm, |);
            EXEC_REGIMM_OPC(IR_OPCODE_ANDI, INS->rd, INS->rs1, INS->imm, &);
            EXEC_REGIMM_OPC(IR_OPCODE_SLLI, INS->rd, INS->rs1, INS->imm, <<);
            EXEC_REGIMM_OPC(IR_OPCODE_SRLI, INS->rd, INS->rs1, INS->imm, >>);

        case IR_OPCODE_SRAI: 
            REG[INS->rd] = ((int32_t)REG[INS->rs1]) >> INS->imm;
            break; 

        case IR_OPCODE_SLTI: 
            REG[INS->rd] = (((int32_t)REG[INS->rs1]) < ((int32_t)INS->imm) ? 1 : 0);
            break; 
        case IR_OPCODE_SLTIU: 
            REG[INS->rd] = ((REG[INS->rs1]) < (INS->imm) ? 1 : 0);
            break; 

            //=================

        case IR_OPCODE_LB:
            REG[INS->rd] = *((int8_t *)fastptr8(REG[INS->rs1] + INS->imm));
            break;
        
        case IR_OPCODE_LH: 
            REG[INS->rd] = *((int16_t *)fastptr16(REG[INS->rs1] + INS->imm));
            break;
        
        case IR_OPCODE_LW: 
            REG[INS->rd] = *fastptr32(REG[INS->rs1] + INS->imm);;
            break;
        
        case IR_OPCODE_LBU: 
            REG[INS->rd] = *fastptr8(REG[INS->rs1] + INS->imm);;
            break;
        
        case IR_OPCODE_LHU: 
            REG[INS->rd] = *fastptr16(REG[INS->rs1] + INS->imm);;
            break;
        

        //=================
        case IR_OPCODE_SB:
        { 
            *fastptr8(REG[INS->rs1] + INS->imm) = REG[INS->rs2];
            break;
        }
        case IR_OPCODE_SH:
        { 
            *fastptr16(REG[INS->rs1] + INS->imm) = REG[INS->rs2];
            break;
        }
        case IR_OPCODE_SW:
        { 
            *fastptr32(REG[INS->rs1] + INS->imm) = REG[INS->rs2];
            break;
        }
            //=================

            EXEC_BRUNCH(IR_OPCODE_BEQ, INS->rs1, INS->rs2, ==);
            EXEC_BRUNCH(IR_OPCODE_BNE, INS->rs1, INS->rs2, !=);
            EXEC_BRUNCH(IR_OPCODE_BLTU, INS->rs1, INS->rs2, <);
            EXEC_BRUNCH(IR_OPCODE_BGEU, INS->rs1, INS->rs2, >=);

        case IR_OPCODE_BLT: 
            if (((int32_t)REG[INS->rs1] < (int32_t)REG[INS->rs2]))
            {
                *next_pc = INS->imm;
                return EXEC_JUMP_OUT;
            }
            break;

        case IR_OPCODE_BGE: 
            if (((int32_t)REG[INS->rs1] >= (int32_t)REG[INS->rs2]))
            {
                *next_pc = INS->imm;
                return EXEC_JUMP_OUT;
            }
            break;

        case IR_OPCODE_JAL:
            REG[INS->rd] = INS->rs1;
            *next_pc = INS->imm;
            return EXEC_JUMP_OUT;

        case IR_OPCODE_JALR:

            *next_pc = (REG[INS->rs1] + INS->imm) & ~0b1;
            REG[INS->rd] = INS->rs2;
            return EXEC_JUMP_OUT;

        case IR_OPCODE_LUI: 
        case IR_OPCODE_AUIPC: 
            REG[INS->rd] = INS->imm;
            break; 

        case IR_OPCODE_CSRRW:
        {
            uint32_t t = 0;
            CSR_Read(INS->imm, &t);
            CSR_Write(INS->imm, REG[INS->rs1]);
            REG[INS->rd] = t;
            break;
        }
        case IR_OPCODE_CSRRS:
        {
            uint32_t t = 0;
            CSR_Read(INS->imm, &t);
            CSR_Write(INS->imm, t | REG[INS->rs1]);
            REG[INS->rd] = t;
            break;
        }
        case IR_OPCODE_CSRRC:
        {
            uint32_t t = 0;
            CSR_Read(INS->imm, &t);
            CSR_Write(INS->imm, t & (~REG[INS->rs1]));
            REG[INS->rd] = t;
            break;
        }
        case IR_OPCODE_CSRRWI:
        {
            uint32_t t = 0;
            CSR_Read(INS->imm, &t);
            CSR_Write(INS->imm, INS->rs1);
            REG[INS->rd] = t;
            break;
        }
        case IR_OPCODE_CSRRSI:
        {
            uint32_t t = 0;
            CSR_Read(INS->imm, &t);
            CSR_Write(INS->imm, t | INS->rs1);
            REG[INS->rd] = t;
            break;
        }
        case IR_OPCODE_CSRRCI:
        {
            uint32_t t = 0;
            CSR_Read(INS->imm, &t);
            CSR_Write(INS->imm, t & (~INS->rs1));
            REG[INS->rd] = t;
            break;
        }

        case IR_OPCODE_SFENCE_VMA:
        {
            printf("sfence.vma:%08x,%08x\n", REG[INS->rs1], REG[INS->rs2]);
            break;
        }

        case IR_OPCODE_WFI:
        {

            break;
        }

            // RV32A

        case IR_OPCODE_LR_W:
        { 
            REG[INS->rd] = *fastptr32(REG[INS->rs1]);
            break;
        }
        case IR_OPCODE_SC_W:
        {
            REG[INS->rs1] = *fastptr32(REG[INS->rs2]); 
            break;
        }
        case IR_OPCODE_AMOSWAP_W:
        {
            
            if(INS->rd)
                REG[INS->rd] = *fastptr32(REG[INS->rs1]);
            *fastptr32(REG[INS->rs1]) = REG[INS->rs2]; 
            break;
        }
        case IR_OPCODE_AMOADD_W:
        {
            
            if(INS->rd)
                REG[INS->rd] = *fastptr32(REG[INS->rs1]);
            *fastptr32(REG[INS->rs1]) = (int32_t)REG[INS->rd] + REG[INS->rs2]; 
            break;
        }
        case IR_OPCODE_AMOAND_W:
        {
            
            if(INS->rd)
                REG[INS->rd] = *fastptr32(REG[INS->rs1]);
            *fastptr32(REG[INS->rs1]) = (uint32_t)REG[INS->rd] & REG[INS->rs2]; 
            break;
        }
        case IR_OPCODE_AMOOR_W:
        {   
            
            if(INS->rd)
                REG[INS->rd] = *fastptr32(REG[INS->rs1]);
            *fastptr32(REG[INS->rs1]) = (uint32_t)REG[INS->rd] | REG[INS->rs2]; 
            break;
        }
        case IR_OPCODE_AMOXOR_W:
        {
            
            if(INS->rd)
                REG[INS->rd] = *fastptr32(REG[INS->rs1]);
            *fastptr32(REG[INS->rs1]) = (uint32_t)REG[INS->rd] ^ REG[INS->rs2]; 
            break;
        }
        case IR_OPCODE_AMOMAX_W:
        {
            
            if(INS->rd)
                REG[INS->rd] = *fastptr32(REG[INS->rs1]);
            *fastptr32(REG[INS->rs1]) = __max((int32_t)REG[INS->rd] , REG[INS->rs2]); 
            break;
        }
        case IR_OPCODE_AMOMIN_W:
        {
            
            if(INS->rd)
                REG[INS->rd] = *fastptr32(REG[INS->rs1]);
            *fastptr32(REG[INS->rs1]) = __min((int32_t)REG[INS->rd] , REG[INS->rs2]); 
            break;
        }

        default:
            printf("ERROR OPC :%03d at %08x\n", INS->opcode, CurPC);
            return -1;
        }

        return EXEC_CONTIUNE;
}

#ifdef HYP_JIT

#include <setjmp.h>
jmp_buf jbuf;
void jit_switch_to_sim_line(IRCode *INS, DecodeBlock *DB, uint32_t cur_soff)
{ 
    if(cur_soff >= DB->IR_Length)
    {
        longjmp(jbuf, 1);
    }  
    int ret =  ExecOneLine(INS, &CurPC);
    if(ret == EXEC_JUMP_OUT)
    {
        longjmp(jbuf, 2);
    } 

}

void jit_ret()
{
    longjmp(jbuf, 1);
}

int jit_code_compile(DecodeBlock *DB)
{
    DB->jit_jmp_tab = malloc((DB->IR_Length) * sizeof(uint32_t));
    if(!DB->jit_jmp_tab )
    {
        printf("jit_jmp_tab malloc nomem!\r\n");
        return -1;
    }
    uint32_t jit_code_buf_sz = 512;
    DB->jit_code = malloc(jit_code_buf_sz);
    if(!DB->jit_code )
    {
        printf("jit_code malloc nomem!\r\n");
        return -1;
    }
    
    arm_inst_buf instbuf;
    arm_inst_buf_init(instbuf, DB->jit_code, ((uint32_t)DB->jit_code + jit_code_buf_sz));


    IRCode *INS;
    uint32_t s_off = 0;
    
    #define REG_PRT  ARM_R9
    
    while (s_off < DB->IR_Length)
    {
        INS = &DB->Ins[s_off];
        if((instbuf.limit - instbuf.current) > ((sizeof(uint32_t)) * 20))
        {
            DB->jit_jmp_tab[s_off] = (uint32_t)instbuf.current - (uint32_t)DB->jit_code;
            switch (INS->opcode)
            {


                case IR_OPCODE_SRA:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rs2)
                        arm_load_membase(instbuf, ARM_R7, REG_PRT, INS->rs2 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R7, 0);   

                    if(INS->rd)
                    {
                        arm_shift_reg_reg(instbuf, ARM_SAR, ARM_R5, ARM_R6, ARM_R7);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;

                case IR_OPCODE_SRL:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rs2)
                        arm_load_membase(instbuf, ARM_R7, REG_PRT, INS->rs2 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R7, 0);   

                    if(INS->rd)
                    {
                        arm_shift_reg_reg(instbuf, ARM_SHR, ARM_R5, ARM_R6, ARM_R7);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;

                case IR_OPCODE_SLL:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rs2)
                        arm_load_membase(instbuf, ARM_R7, REG_PRT, INS->rs2 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R7, 0);   

                    if(INS->rd)
                    {
                        
                        arm_shift_reg_reg(instbuf, ARM_SHL, ARM_R5, ARM_R6, ARM_R7);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;

                case IR_OPCODE_AND:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rs2)
                        arm_load_membase(instbuf, ARM_R7, REG_PRT, INS->rs2 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R7, 0);   

                    if(INS->rd)
                    {
                        arm_alu_reg_reg(instbuf, ARM_AND, ARM_R5, ARM_R6, ARM_R7);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;

                case IR_OPCODE_OR:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rs2)
                        arm_load_membase(instbuf, ARM_R7, REG_PRT, INS->rs2 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R7, 0);   

                    if(INS->rd)
                    {
                        arm_alu_reg_reg(instbuf, ARM_ORR, ARM_R5, ARM_R6, ARM_R7);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;

                case IR_OPCODE_XOR:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rs2)
                        arm_load_membase(instbuf, ARM_R7, REG_PRT, INS->rs2 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R7, 0);   

                    if(INS->rd)
                    {
                        arm_alu_reg_reg(instbuf, ARM_EOR, ARM_R5, ARM_R6, ARM_R7);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;

                case IR_OPCODE_SUB:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rs2)
                        arm_load_membase(instbuf, ARM_R7, REG_PRT, INS->rs2 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R7, 0);   

                    if(INS->rd)
                    {
                        arm_alu_reg_reg(instbuf, ARM_SUB, ARM_R5, ARM_R6, ARM_R7);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;

                case IR_OPCODE_ADD:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rs2)
                        arm_load_membase(instbuf, ARM_R7, REG_PRT, INS->rs2 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R7, 0);   

                    if(INS->rd)
                    {
                        arm_alu_reg_reg(instbuf, ARM_ADD, ARM_R5, ARM_R6, ARM_R7);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;
                    
                case IR_OPCODE_ADDI:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rd)
                    {
                        arm_alu_reg_imm(instbuf, ARM_ADD, ARM_R5, ARM_R6, INS->imm);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;
                case IR_OPCODE_XORI:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rd)
                    {
                        arm_alu_reg_imm(instbuf, ARM_EOR, ARM_R5, ARM_R6, INS->imm);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;
                case IR_OPCODE_ORI:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rd)
                    {
                        arm_alu_reg_imm(instbuf, ARM_ORR, ARM_R5, ARM_R6, INS->imm);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;
                case IR_OPCODE_ANDI:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rd)
                    {
                        arm_alu_reg_imm(instbuf, ARM_AND, ARM_R5, ARM_R6, INS->imm);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;
                case IR_OPCODE_SLLI:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rd)
                    {
                        arm_shift_reg_imm8(instbuf, ARM_SHL, ARM_R5, ARM_R6, INS->imm);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;
                case IR_OPCODE_SRLI:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rd)
                    {
                        arm_shift_reg_imm8(instbuf, ARM_SHR, ARM_R5, ARM_R6, INS->imm);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;
                case IR_OPCODE_SRAI:
                    
                    //arm_store_membase(instbuf, ARM_R4, REG_PRT, 0);
                    if(INS->rs1)
                        arm_load_membase(instbuf, ARM_R6, REG_PRT, INS->rs1 * 4);
                    else
                        arm_mov_reg_imm8(instbuf, ARM_R6, 0);

                    if(INS->rd)
                    {
                        arm_shift_reg_imm8(instbuf, ARM_SAR, ARM_R5, ARM_R6, INS->imm);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;
                
                case IR_OPCODE_LUI:
                case IR_OPCODE_AUIPC:
                    if(INS->rd)
                    {
                        arm_mov_reg_imm(instbuf, ARM_R5, INS->imm);
                        arm_store_membase(instbuf, ARM_R5, REG_PRT, INS->rd * 4);
                    }
                    break;
                    //for(uint32_t *s = (uint32_t *)(DB->jit_jmp_tab[s_off] + (uint32_t)DB->jit_code);
                    //        s < (uint32_t *)instbuf.current; s++)
                    //        {
                    //            printf("%08lX\r\n", *s);
                    //        }
                   
                default:
                    arm_mov_reg_imm(instbuf, ARM_R0, INS);
                    arm_mov_reg_imm(instbuf, ARM_R1, DB);
                    arm_mov_reg_imm(instbuf, ARM_R2, s_off);
                    arm_call(instbuf, jit_switch_to_sim_line);
            }
            if(instbuf.limit - instbuf.current < 0)
            {
                printf("!!!JIT BUF OF!\r\n");
            }
            s_off++;
        }else{
            //printf("ext jit buf\r\n");
            jit_code_buf_sz += 128;
            instbuf.current = (arm_inst_word *)((uint32_t)instbuf.current - (uint32_t)DB->jit_code);
            DB->jit_code = realloc(DB->jit_code, jit_code_buf_sz);
            instbuf.current = (arm_inst_word *)((uint32_t)instbuf.current + (uint32_t)DB->jit_code);
            instbuf.limit = (arm_inst_word *)((uint32_t)DB->jit_code + jit_code_buf_sz);
            if(!DB->jit_code)
            {
                printf("!!!NO MEM!\r\n");
            }
        }
    }
    arm_call(instbuf, jit_ret);
    llapi_clean_dcache((void *)DB->jit_code, jit_code_buf_sz);
    

}

void __attribute__((naked))  CallJitCode(void *addr) 
{
  // __asm volatile("push {r1-r12,lr}"); 
   __asm volatile("ldr r9,=REG");
   __asm volatile("mov r4,#0");
   __asm volatile("bx r0"); 
  // __asm volatile("pop {r1-r12,pc}");
}
int jit_code_exec(DecodeBlock *DB, uint32_t offset)
{
    int ret = 0;
    uint32_t s_off = 0;
    void (*f)();

    llapi_invalidate_icache();
    //ret = setjmp_(cjmpbuf, 0);
    ret = setjmp(jbuf);
    //printf("jret:%d\r\n", ret);
    switch (ret)
    {
    case 0:
        s_off = offset / DB->src_ins_len; 
        f = (void *)(DB->jit_jmp_tab[s_off] + (uint32_t)DB->jit_code);
        //f();
        CallJitCode((void *)f); 
        break;
    case 1:
        //printf("next pc:%08lX\r\n", CurPC);
        
        Instructions += DB->IR_Length;
        CurPC = DB->src_pc + DB->src_ins_len * DB->IR_Length;  
        return 0; 
    case EXEC_JUMP_OUT:
        
        //printf("next pc:%08lX\r\n", CurPC);
        return 0; 
    default:
        printf("ERR RET:%d\r\n", ret);
        break;
    }

}
#endif


 #define unlikely(x) __builtin_expect((x), 0)
 #define likely(x) __builtin_expect((x), 1)
 
static int inline IRCodeExec(DecodeBlock *DB, uint32_t offset)
{

    IRCode *INS = &DB->Ins[0]; 
    uint32_t s_off = 0;

    s_off = offset / DB->src_ins_len;
    INS = &DB->Ins[s_off]; 
    //if(DB->IR_Length > 67)
    //printf("exec len:%ld\r\n", DB->IR_Length); 
    while (s_off < DB->IR_Length)
    {
        

        //CurPC = DB->src_pc + s_off * DB->src_ins_len;
        // printf("PC:%08x | ", DB->src_pc + s_off * DB->src_ins_len );
        // dump_IRCode(*INS);
        
        
        if(unlikely(ExecOneLine(INS, &CurPC) == EXEC_JUMP_OUT))
        {
            Instructions+=(s_off - offset / DB->src_ins_len);
            return 0;
        }

        s_off++;
        INS = &DB->Ins[s_off];
    }

    
    Instructions+=(s_off - offset / DB->src_ins_len);
    //CurPC += DB->src_ins_len;
    CurPC = DB->src_pc + DB->src_ins_len * DB->IR_Length; 
    return 0;
}




void dump_registers()
{
    for (int i = 0; i < 32; i++)
    {
        printf("X%d = %08x\n", i, REG[i]);
    }
    printf("PC = %08x\n", CurPC);
}

uint32_t miss = 0, hit = 0, loop_cnt = 0;

void *ThreadCheck(void *arg)
{
#if defined(_WIN64) || defined(_WIN32)
    while (1)
    {
        _sleep(1);
#else
    while(1)
    {
#endif
        printf("Freq:%.2f MIPS, acc:%f, loop_cnt:%d\n",
               Instructions / 1e6f / 2,
               ((float)hit / (miss + hit)),
               loop_cnt / 2);
        Instructions = 0;
        miss = hit = loop_cnt = 0;
        llapi_delay_ms(2000);
#if defined(_WIN64) || defined(_WIN32)
        if (kbhit())
        {
            int c = getch();
            // printf("c:%d\n", c);
            if (c == 'r')
            {
                dump_registers();
            }
        }
#endif
    }
    return NULL;
}

void simulatorStart(uint32_t pc)
{
    int ret = 0;
    int run = 1, inloop = 0;
    DecodeBlock *DB;
    uint32_t offset, next_pc;

    memset(REG, 0, sizeof(REG));

#if (defined(_WIN64) || defined(_WIN32))
    pthread_t th;
    pthread_create(&th, NULL, ThreadCheck, NULL);
    printf("Create thread.\n");
#endif
    
    llapi_thread_create(ThreadCheck, malloc(1024), 1024, NULL);
    CurPC = pc;
    // DecodeSrc(0);
    while (run)
    {
#if !(defined(_WIN64) || defined(_WIN32)) && 0
        static uint32_t tm1 = 0;
        if ((llapi_get_tick_ms() - tm1) > 1000)
        {
            tm1 = llapi_get_tick_ms();
            ThreadCheck(NULL);
        }
#endif

        if (GetCachedDBByAddr(CurPC, &DB, &offset) == -1)
        {
            miss++;
            ret = DecodeSrc(CurPC);
            if (ret == -1)
            {
                run = 0;
                printf("Decode ERROR\n");
                dump_registers();
                exit(-1);
            }
        }
        else
        {
            uint32_t DB_pc_lower = DB->src_pc;
            uint32_t DB_pc_upper = DB->src_pc + DB->src_codes_length;
            #ifdef HYP_JIT
            if(!DB->jit_code)
                jit_code_compile(DB);
            #endif

            //printf("l:%d\r\n",DB->IR_Length);
            do
            {
                hit++;
                // DB->exec_cnt++;
                
                #ifdef HYP_JIT
                    ret = jit_code_exec(DB, offset);
                #else
                    ret = IRCodeExec(DB, offset);
                #endif
                //
                if (ret == -1)
                {
                    run = 0;
                    printf("Running ERROR\n");
                    dump_registers();
                    exit(-1);
                }
                if(stdout_port)
                {
                    putchar(stdout_port);
                    stdout_port = 0;
                }
 
                // printf("%08x\n",pc);
                if ((CurPC >= DB_pc_lower) && (CurPC < DB_pc_upper))
                {
                    offset = CurPC - DB_pc_lower;
                    //inloop = 1;
                    loop_cnt++;
                    if (loop_cnt > 1000000)
                    {
                        inloop = 0;
                    }
                    else
                    {
                        inloop = 1;
                    }
                }
                else
                {
                    inloop = 0;
                }
            } while (inloop);
        }
    }
}
