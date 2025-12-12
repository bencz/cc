/*
 * backend_hlasm.c - Backend HLASM para Mainframe
 * 
 * Este módulo gera código assembler HLASM (High Level Assembler) 
 * para mainframes IBM z/OS a partir do código intermediário.
 * 
 * Uso: compile com -DUSE_HLASM_BACKEND e use a opção -hlasm
 */

#include "cc.h"

#ifdef USE_HLASM_BACKEND

#include <stdarg.h>

/*============================================================================
 * Registradores z/Architecture
 * 
 * R0-R15: Registradores de propósito geral (GPR)
 * F0-F15: Registradores de ponto flutuante (FPR)
 * 
 * Convenções típicas:
 * R1:  Parâmetros
 * R13: Save area pointer
 * R14: Return address
 * R15: Entry point / return code
 *============================================================================*/

typedef enum {
    R0 = 0, R1, R2, R3, R4, R5, R6, R7,
    R8, R9, R10, R11, R12, R13, R14, R15
} GPR;

/*============================================================================
 * Estrutura para instrução HLASM
 *============================================================================*/

typedef struct _HLASM_INST {
    int  opcode;
    char *mnemonic;
    char *format;      /* RR, RX, RS, SI, SS, etc */
    int  length;       /* 2, 4, or 6 bytes */
} HLASM_INST;

/*============================================================================
 * Tabela de Instruções HLASM (subset)
 * 
 * Formato das instruções z/Architecture:
 * - RR: Register-Register (2 bytes)
 * - RX: Register-Index-Base-Displacement (4 bytes)
 * - RS: Register-Register-Storage (4 bytes)
 * - SI: Storage-Immediate (4 bytes)
 * - SS: Storage-Storage (6 bytes)
 *============================================================================*/

static HLASM_INST hlasm_inst[] = {
    /* Instruções de Load/Store */
    { 0x18, "LR",    "RR", 2 },    /* Load Register */
    { 0x58, "L",     "RX", 4 },    /* Load */
    { 0x50, "ST",    "RX", 4 },    /* Store */
    { 0x48, "LH",    "RX", 4 },    /* Load Halfword */
    { 0x40, "STH",   "RX", 4 },    /* Store Halfword */
    { 0x43, "IC",    "RX", 4 },    /* Insert Character */
    { 0x42, "STC",   "RX", 4 },    /* Store Character */
    { 0x41, "LA",    "RX", 4 },    /* Load Address */
    
    /* Instruções Aritméticas */
    { 0x1A, "AR",    "RR", 2 },    /* Add Register */
    { 0x5A, "A",     "RX", 4 },    /* Add */
    { 0x1B, "SR",    "RR", 2 },    /* Subtract Register */
    { 0x5B, "S",     "RX", 4 },    /* Subtract */
    { 0x1C, "MR",    "RR", 2 },    /* Multiply Register */
    { 0x5C, "M",     "RX", 4 },    /* Multiply */
    { 0x1D, "DR",    "RR", 2 },    /* Divide Register */
    { 0x5D, "D",     "RX", 4 },    /* Divide */
    
    /* Instruções Lógicas */
    { 0x14, "NR",    "RR", 2 },    /* AND Register */
    { 0x54, "N",     "RX", 4 },    /* AND */
    { 0x16, "OR",    "RR", 2 },    /* OR Register */
    { 0x56, "O",     "RX", 4 },    /* OR */
    { 0x17, "XR",    "RR", 2 },    /* XOR Register */
    { 0x57, "X",     "RX", 4 },    /* XOR */
    
    /* Instruções de Comparação */
    { 0x19, "CR",    "RR", 2 },    /* Compare Register */
    { 0x59, "C",     "RX", 4 },    /* Compare */
    { 0x15, "CLR",   "RR", 2 },    /* Compare Logical Register */
    { 0x55, "CL",    "RX", 4 },    /* Compare Logical */
    
    /* Instruções de Branch */
    { 0x47, "BC",    "RX", 4 },    /* Branch on Condition */
    { 0x45, "BAL",   "RX", 4 },    /* Branch and Link */
    { 0x05, "BALR",  "RR", 2 },    /* Branch and Link Register */
    { 0x07, "BCR",   "RR", 2 },    /* Branch on Condition Register */
    
    /* Instruções de Shift */
    { 0x8A, "SRA",   "RS", 4 },    /* Shift Right Arithmetic */
    { 0x8B, "SLA",   "RS", 4 },    /* Shift Left Arithmetic */
    { 0x88, "SRL",   "RS", 4 },    /* Shift Right Logical */
    { 0x89, "SLL",   "RS", 4 },    /* Shift Left Logical */
    
    /* Fim da tabela */
    { 0, NULL, NULL, 0 }
};

/*============================================================================
 * Máscaras de Condição para Branch
 *============================================================================*/

#define MASK_NEVER    0   /* Never branch */
#define MASK_OVERFLOW 1   /* Overflow */
#define MASK_HIGH     2   /* High (Greater than) */
#define MASK_LOW      4   /* Low (Less than) */
#define MASK_EQUAL    8   /* Equal */
#define MASK_NOTHIGH  (MASK_LOW | MASK_EQUAL)      /* Not High (<=) */
#define MASK_NOTLOW   (MASK_HIGH | MASK_EQUAL)     /* Not Low (>=) */
#define MASK_NOTEQUAL (MASK_HIGH | MASK_LOW)       /* Not Equal */
#define MASK_ALWAYS   15  /* Always branch */

/*============================================================================
 * Buffer de Saída HLASM
 *============================================================================*/

static FILE *hlasm_output = NULL;
static int   hlasm_label_counter = 0;
static char  hlasm_csect_name[9] = "CCPROG";

/*============================================================================
 * Funções de Inicialização
 *============================================================================*/

void hlasm_init(const char *output_file)
{
    hlasm_output = fopen(output_file, "w");
    if (hlasm_output == NULL) {
        error("hlasm_init", "Cannot open output file '%s'", output_file);
    }
    hlasm_label_counter = 0;
}

void hlasm_close(void)
{
    if (hlasm_output != NULL) {
        fclose(hlasm_output);
        hlasm_output = NULL;
    }
}

/*============================================================================
 * Funções de Emissão de Código
 *============================================================================*/

/* Emite uma linha de código HLASM */
static void emit(const char *fmt, ...)
{
    va_list args;
    char buffer[256];
    
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    if (hlasm_output != NULL) {
        fprintf(hlasm_output, "%s\n", buffer);
    }
}

/* Emite um label */
static void emit_label(const char *label)
{
    emit("%-8s DS    0H", label);
}

/* Emite instrução RR */
static void emit_rr(const char *mnemonic, int r1, int r2)
{
    emit("         %-5s %d,%d", mnemonic, r1, r2);
}

/* Emite instrução RX */
static void emit_rx(const char *mnemonic, int r1, int disp, int index, int base)
{
    if (index == 0) {
        emit("         %-5s %d,%d(,%d)", mnemonic, r1, disp, base);
    } else {
        emit("         %-5s %d,%d(%d,%d)", mnemonic, r1, disp, index, base);
    }
}

/* Emite instrução com label */
static void emit_rx_label(const char *mnemonic, int r1, const char *label)
{
    emit("         %-5s %d,%s", mnemonic, r1, label);
}

/* Gera um label único */
static int new_label(void)
{
    return ++hlasm_label_counter;
}

/* Formata um label */
static void format_label(char *buf, int label_num)
{
    sprintf(buf, "L%04d", label_num);
}

/*============================================================================
 * Geração de Prólogo/Epílogo de Função
 *============================================================================*/

void hlasm_function_prolog(const char *func_name)
{
    emit("*");
    emit("* Function: %s", func_name);
    emit("*");
    emit("%-8s DS    0H", func_name);
    emit("         STM   14,12,12(13)      Save registers");
    emit("         LR    12,15             Establish base");
    emit("         USING %s,12", func_name);
    emit("         LA    14,SAVEAREA       Get save area");
    emit("         ST    13,4(14)          Chain save areas");
    emit("         ST    14,8(13)");
    emit("         LR    13,14             New save area");
}

void hlasm_function_epilog(void)
{
    emit("         L     13,4(13)          Restore save area");
    emit("         LM    14,12,12(13)      Restore registers");
    emit("         BR    14                Return");
}

/*============================================================================
 * Tradução de Código Intermediário para HLASM
 * 
 * Esta função traduz o código intermediário (cd.pCode) para HLASM.
 * Cada instrução intermediária é mapeada para uma ou mais instruções HLASM.
 *============================================================================*/

void hlasm_translate(void)
{
    int n;
    char label_buf[16];
    
    /* Cabeçalho do programa */
    emit("*");
    emit("* Generated by CC Compiler - HLASM Backend");
    emit("*");
    emit("%-8s CSECT", hlasm_csect_name);
    emit("         USING *,12");
    emit("*");
    
    /* Traduz cada instrução */
    for (n = 0; n < ix.ixCode; n++) {
        INSTRUCT *pInst = &cd.pCode[n];
        int inst = pInst->inst;
        int num = pInst->num;
        
        /* Pula instruções de dados */
        if (inst >= setint && inst <= setstr) continue;
        
        switch (inst) {
            case fn_:
            case exp_:
                /* Início de função */
                hlasm_function_prolog(toString(num));
                break;
                
            case xret:
                /* Retorno de função */
                hlasm_function_epilog();
                break;
                
            case loc_:
                /* Label */
                format_label(label_buf, num);
                emit_label(label_buf);
                break;
                
            case xent:
                /* Entry point - já tratado em fn_ */
                break;
                
            case inc_dbp:
                /* Increment dword at [ebp+offset] */
                emit("         L     0,%d(,11)", num + 72);
                emit("         AHI   0,1");
                emit("         ST    0,%d(,11)", num + 72);
                break;
                
            case dec_dbp:
                /* Decrement dword at [ebp+offset] */
                emit("         L     0,%d(,11)", num + 72);
                emit("         AHI   0,-1");
                emit("         ST    0,%d(,11)", num + 72);
                break;
                
            case mov_eax:
                /* Load immediate */
                if (num >= 0 && num < 4096) {
                    emit("         LA    2,%d", num);
                } else {
                    emit("         L     2,=F'%d'", num);
                }
                break;
                
            case mov_ecx:
                if (num >= 0 && num < 4096) {
                    emit("         LA    3,%d", num);
                } else {
                    emit("         L     3,=F'%d'", num);
                }
                break;
                
            case mov_eax_pbp:
                /* Load from stack frame */
                emit("         L     2,%d(,11)", num + 72);
                break;
                
            case mov_ecx_pbp:
                emit("         L     3,%d(,11)", num + 72);
                break;
                
            case mov_edx_pbp:
                emit("         L     4,%d(,11)", num + 72);
                break;
                
            case mov_pbp_eax:
                /* Store to stack frame */
                emit("         ST    2,%d(,11)", num + 72);
                break;
                
            case lea_eax_pbp:
                /* Load address from stack */
                emit("         LA    2,%d(,11)", num + 72);
                break;
                
            case mov_eax_pax:
                /* Load indirect */
                emit("         L     2,0(,2)");
                break;
                
            case mov_eax_ad1:
                /* mov eax,[eax+edx] - array access scale 1 */
                emit("         AR    2,4");
                emit("         L     2,0(,2)");
                break;
                
            case mov_eax_ad2:
                /* mov eax,[eax+edx*2] - array access scale 2 */
                emit("         SLL   4,1");
                emit("         AR    2,4");
                emit("         L     2,0(,2)");
                break;
                
            case mov_eax_ad4:
                /* mov eax,[eax+edx*4] - array access scale 4 */
                emit("         SLL   4,2");
                emit("         AR    2,4");
                emit("         L     2,0(,2)");
                break;
                
            case mov_eax_ad8:
                /* mov eax,[eax+edx*8] - array access scale 8 */
                emit("         SLL   4,3");
                emit("         AR    2,4");
                emit("         L     2,0(,2)");
                break;
                
            case lea_eax_ad1:
                /* lea eax,[eax+edx] */
                emit("         AR    2,4");
                break;
                
            case lea_eax_ad4:
                /* lea eax,[eax+edx*4] */
                emit("         SLL   4,2");
                emit("         AR    2,4");
                break;
                
            case mov_pcx_eax:
                /* Store indirect */
                emit("         ST    2,0(,3)");
                break;
                
            case mov_pcx_ax:
                /* Store halfword at [ecx] */
                emit("         STH   2,0(,3)");
                break;
                
            case mov_pcx_al:
                /* Store byte at [ecx] */
                emit("         STC   2,0(,3)");
                break;
                
            case add_eax_ecx:
                emit_rr("AR", 2, 3);
                break;
                
            case add_eax:
                if (num >= 0 && num < 4096) {
                    emit("         AHI   2,%d", num);
                } else {
                    emit("         A     2,=F'%d'", num);
                }
                break;
                
            case sub_eax_ecx:
                emit_rr("SR", 2, 3);
                break;
                
            case sub_eax:
                if (num >= 0 && num < 4096) {
                    emit("         AHI   2,-%d", num);
                } else {
                    emit("         S     2,=F'%d'", num);
                }
                break;
                
            case imul_eax_ecx:
                /* z/Arch multiply uses even-odd pair */
                emit("         LR    4,2");
                emit("         MR    4,3");
                emit("         LR    2,5");
                break;
                
            case imul_eax_eax:
                emit("         MHI   2,%d", num);
                break;
                
            case imul_eax_pbp:
                emit("         M     2,%d(,11)", num + 72);
                emit("         LR    2,3");
                break;
                
            case xdiv_dbp:
                /* divide eax by [ebp+offset] */
                emit("         LR    4,2");
                emit("         SRDA  4,32");
                emit("         D     4,%d(,11)", num + 72);
                emit("         LR    2,5");
                break;
                
            case and_eax_ecx:
                emit_rr("NR", 2, 3);
                break;
                
            case or_eax_ecx:
                emit_rr("OR", 2, 3);
                break;
                
            case xor_eax_ecx:
                emit_rr("XR", 2, 3);
                break;
                
            case cmp_eax:
                emit("         CHI   2,%d", num);
                break;
                
            case cmp_eax_ecx:
                emit_rr("CR", 2, 3);
                break;
                
            case cmp_ecx_eax:
                emit_rr("CR", 3, 2);
                break;
                
            case test_eax_eax:
                emit("         LTR   2,2");
                break;
                
            case jmp:
                format_label(label_buf, num);
                emit("         B     %s", label_buf);
                break;
                
            case jz:
                format_label(label_buf, num);
                emit("         BZ    %s", label_buf);
                break;
                
            case jnz:
                format_label(label_buf, num);
                emit("         BNZ   %s", label_buf);
                break;
                
            case jl:
                format_label(label_buf, num);
                emit("         BL    %s", label_buf);
                break;
                
            case jge:
                format_label(label_buf, num);
                emit("         BNL   %s", label_buf);
                break;
                
            case jle:
                format_label(label_buf, num);
                emit("         BNH   %s", label_buf);
                break;
                
            case jg:
                format_label(label_buf, num);
                emit("         BH    %s", label_buf);
                break;
                
            case call:
                /* Function call using BALR */
                emit("         LA    15,%s", toString(num));
                emit("         BALR  14,15");
                break;
                
            case push_eax:
                emit("         ST    2,0(,13)");
                emit("         AHI   13,-4");
                break;
                
            case push_ecx:
                emit("         ST    3,0(,13)");
                emit("         AHI   13,-4");
                break;
                
            case pop_eax:
                emit("         AHI   13,4");
                emit("         L     2,0(,13)");
                break;
                
            case pop_ecx:
                emit("         AHI   13,4");
                emit("         L     3,0(,13)");
                break;
                
            case sub_esp:
                emit("         AHI   13,-%d", num);
                break;
                
            case add_esp:
                emit("         AHI   13,%d", num);
                break;
                
            case sete_eax:
                emit("         LA    2,0");
                emit("         BE    *+6");
                emit("         LA    2,1");
                break;
                
            case setne_eax:
                emit("         LA    2,0");
                emit("         BNE   *+6");
                emit("         LA    2,1");
                break;
                
            case setl_eax:
                emit("         LA    2,0");
                emit("         BL    *+6");
                emit("         LA    2,1");
                break;
                
            case setg_eax:
                emit("         LA    2,0");
                emit("         BH    *+6");
                emit("         LA    2,1");
                break;
                
            case neg_eax:
                emit("         LCR   2,2");
                break;
                
            case xchg_eax_ecx:
                emit("         LR    0,2");
                emit("         LR    2,3");
                emit("         LR    3,0");
                break;
                
            case mov_ecx_eax:
                emit_rr("LR", 3, 2);
                break;
                
            case mov_edx_eax:
                emit_rr("LR", 4, 2);
                break;
                
            case mov_eax_edx:
                emit_rr("LR", 2, 4);
                break;
                
            case pop_edx:
                emit("         AHI   13,4");
                emit("         L     4,0(,13)");
                break;
                
            case push_pbp:
                /* push [ebp+offset] */
                emit("         L     0,%d(,11)", num + 72);
                emit("         ST    0,0(,13)");
                emit("         AHI   13,-4");
                break;
                
            case push:
                /* push immediate */
                emit("         L     0,=F'%d'", num);
                emit("         ST    0,0(,13)");
                emit("         AHI   13,-4");
                break;
                
            case add_eax_edx:
                emit_rr("AR", 2, 4);
                break;
                
            case add_ecx:
                if (num >= 0 && num < 4096) {
                    emit("         AHI   3,%d", num);
                } else {
                    emit("         A     3,=F'%d'", num);
                }
                break;
                
            case add_eax_pbp:
                emit("         A     2,%d(,11)", num + 72);
                break;
                
            case add_pbp_eax:
                emit("         L     0,%d(,11)", num + 72);
                emit("         AR    0,2");
                emit("         ST    0,%d(,11)", num + 72);
                break;
                
            case sub_eax_pbp:
                emit("         S     2,%d(,11)", num + 72);
                break;
                
            case sub_pbp_eax:
                emit("         L     0,%d(,11)", num + 72);
                emit("         SR    0,2");
                emit("         ST    0,%d(,11)", num + 72);
                break;
                
            case cmp_eax_pbp:
                emit("         C     2,%d(,11)", num + 72);
                break;
                
            case setge_eax:
                emit("         LA    2,0");
                emit("         BNL   *+6");
                emit("         LA    2,1");
                break;
                
            case setle_eax:
                emit("         LA    2,0");
                emit("         BNH   *+6");
                emit("         LA    2,1");
                break;
                
            case shl_eax_cl:
                /* shift left by CL (R3 low byte) */
                emit("         SLL   2,0(3)");
                break;
                
            case shr_eax_cl:
                /* shift right by CL */
                emit("         SRL   2,0(3)");
                break;
                
            case shl_eax:
                emit("         SLL   2,%d", num);
                break;
                
            case xor_eax_eax:
                emit("         XR    2,2");
                break;
                
            case cwde:
                /* sign extend AX to EAX - in z/Arch just ensure proper sign */
                emit("         LR    2,2");
                break;
                
            case lea_ecx_pbp:
                emit("         LA    3,%d(,11)", num + 72);
                break;
                
            case lea_edx_pbp:
                emit("         LA    4,%d(,11)", num + 72);
                break;
                
            case mov_pbp_ecx:
                emit("         ST    3,%d(,11)", num + 72);
                break;
                
            case mov_ecx_pax:
                emit("         L     3,0(,2)");
                break;
                
            case mov_edx_pax:
                emit("         L     4,0(,2)");
                break;
                
            case mov_pax_ecx:
                emit("         ST    3,0(,2)");
                break;
                
            case mov_pax_cx:
                /* store halfword at [eax] */
                emit("         STH   3,0(,2)");
                break;
                
            case mov_pax_cl:
                /* store byte at [eax] */
                emit("         STC   3,0(,2)");
                break;
                
            case movsx_eax_bax:
                /* sign extend byte at [eax] to eax */
                emit("         IC    2,0(,2)");
                emit("         SLL   2,24");
                emit("         SRA   2,24");
                break;
                
            case movsx_eax_wax:
                /* sign extend halfword at [eax] to eax */
                emit("         LH    2,0(,2)");
                break;
                
            case mov_pbp_al:
                /* store byte */
                emit("         STC   2,%d(,11)", num + 72);
                break;
                
            case xdiv_ecx:
                /* divide eax by ecx, result in eax */
                emit("         LR    4,2");
                emit("         SRDA  4,32");
                emit("         DR    4,3");
                emit("         LR    2,5");
                break;
                
            case xmod_ecx:
                /* modulo eax by ecx, result in eax */
                emit("         LR    4,2");
                emit("         SRDA  4,32");
                emit("         DR    4,3");
                emit("         LR    2,4");
                break;
                
            default:
                /* Instrução não implementada - emite comentário */
                emit("* TODO: opcode %d (num=%d)", inst, num);
                break;
        }
    }
    
    /* Emite seção de dados */
    emit("*");
    emit("* Data Section");
    emit("*");
    
    for (n = 0; n < ix.ixCode; n++) {
        INSTRUCT *pInst = &cd.pCode[n];
        if (pInst->inst == setint) {
            emit("         DC    F'%d'", pInst->num);
        } else if (pInst->inst == setreal) {
            emit("         DC    D'%f'", pInst->dval);
        } else if (pInst->inst == setstr) {
            emit("         DC    C'%s'", pInst->sval);
        }
    }
    
    emit("*");
    emit("SAVEAREA DS    18F               Register save area");
    emit("         LTORG");
    emit("         END");
}

/*============================================================================
 * Função Principal de Link para HLASM
 * 
 * Esta função substitui link() quando usando o backend HLASM.
 *============================================================================*/

void hlasm_link(const char *output_file)
{
    hlasm_init(output_file);
    hlasm_translate();
    hlasm_close();
    
    printf("HLASM output written to: %s\n", output_file);
}

#endif /* USE_HLASM_BACKEND */

/*============================================================================
 * Notas de Implementação
 * 
 * Para completar este backend, você precisará:
 * 
 * 1. Mapeamento de Registradores:
 *    - Mapear os registradores virtuais (eax, ecx, edx) para GPRs z/Arch
 *    - Considerar convenções de chamada z/OS
 * 
 * 2. Convenções de Chamada:
 *    - z/OS usa R1 para lista de parâmetros
 *    - R13 para save area
 *    - R14 para return address
 *    - R15 para entry point e return code
 * 
 * 3. Gerenciamento de Stack:
 *    - z/Architecture não tem stack hardware como x86
 *    - Usar save areas encadeadas
 * 
 * 4. Ponto Flutuante:
 *    - z/Arch tem FPRs separados (F0-F15)
 *    - Instruções diferentes para float/double
 * 
 * 5. Strings e Dados:
 *    - EBCDIC vs ASCII
 *    - Alinhamento de dados
 * 
 * 6. Chamadas de Sistema:
 *    - Usar macros z/OS (WTO, OPEN, CLOSE, etc.)
 *    - Ou chamar funções C runtime
 *============================================================================*/
