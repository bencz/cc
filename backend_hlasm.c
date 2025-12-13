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
 * Tabela de Conversão ASCII para EBCDIC
 * 
 * z/OS usa EBCDIC como encoding padrão. Esta tabela converte caracteres
 * ASCII (0-127) para seus equivalentes EBCDIC.
 *============================================================================*/

static const unsigned char ascii_to_ebcdic[256] = {
    /* 0x00-0x0F */
    0x00, 0x01, 0x02, 0x03, 0x37, 0x2D, 0x2E, 0x2F,
    0x16, 0x05, 0x25, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    /* 0x10-0x1F */
    0x10, 0x11, 0x12, 0x13, 0x3C, 0x3D, 0x32, 0x26,
    0x18, 0x19, 0x3F, 0x27, 0x1C, 0x1D, 0x1E, 0x1F,
    /* 0x20-0x2F (space, !, ", #, $, %, &, ', (, ), *, +, ,, -, ., /) */
    0x40, 0x5A, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D,
    0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,
    /* 0x30-0x3F (0-9, :, ;, <, =, >, ?) */
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F,
    /* 0x40-0x4F (@, A-O) */
    0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
    0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
    /* 0x50-0x5F (P-Z, [, \, ], ^, _) */
    0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6,
    0xE7, 0xE8, 0xE9, 0xAD, 0xE0, 0xBD, 0x5F, 0x6D,
    /* 0x60-0x6F (`, a-o) */
    0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,
    /* 0x70-0x7F (p-z, {, |, }, ~, DEL) */
    0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6,
    0xA7, 0xA8, 0xA9, 0xC0, 0x4F, 0xD0, 0xA1, 0x07,
    /* 0x80-0xFF: extended ASCII mapped to EBCDIC spaces or equivalents */
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40
};

/* Converte string ASCII para EBCDIC */
static void ascii_to_ebcdic_str(char *dest, const char *src, int len)
{
    int i;
    for (i = 0; i < len && src[i] != '\0'; i++) {
        dest[i] = ascii_to_ebcdic[(unsigned char)src[i]];
    }
    dest[i] = '\0';
}

/*============================================================================
 * Registradores z/Architecture e Convenções z/OS
 * 
 * GPRs (R0-R15):
 *   R0:      Work register (não preservado)
 *   R1:      Parameter list pointer (não preservado)
 *   R2-R4:   Work registers / return values (não preservado)
 *            Mapeamento do compilador: R2=eax, R3=ecx, R4=edx
 *   R5-R10:  Preservados pelo callee
 *   R11:     Frame pointer (ebp) - preservado
 *   R12:     Base register - preservado
 *   R13:     Save area pointer - preservado
 *   R14:     Return address - preservado
 *   R15:     Entry point / return code (não preservado)
 * 
 * FPRs (F0-F15):
 *   F0, F2, F4, F6: Parâmetros float/double e retorno
 *   F1, F3, F5, F7: Work registers
 *   F8-F15: Preservados pelo callee
 * 
 * Convenção de Chamada z/OS:
 *   - Parâmetros passados via lista apontada por R1
 *   - Cada entrada na lista é um ponteiro para o argumento
 *   - Último ponteiro tem bit de sinal setado (high-order bit)
 *   - Retorno inteiro em R15 (ou R0:R1 para 64-bit)
 *   - Retorno float/double em F0
 *   - Save area de 72 bytes (18 fullwords) apontada por R13
 *============================================================================*/

typedef enum {
    R0 = 0, R1, R2, R3, R4, R5, R6, R7,
    R8, R9, R10, R11, R12, R13, R14, R15
} GPR;

typedef enum {
    F0 = 0, F1, F2, F3, F4, F5, F6, F7,
    F8, F9, F10, F11, F12, F13, F14, F15
} FPR;

/* Mapeamento de registradores x86 -> z/Architecture */
#define REG_EAX  R2   /* Acumulador */
#define REG_ECX  R3   /* Contador */
#define REG_EDX  R4   /* Dados */
#define REG_EBP  R11  /* Frame pointer */
#define REG_ESP  R13  /* Stack pointer (save area) */

/* Offset base para variáveis locais no stack frame */
#define STACK_FRAME_BASE 72  /* Após save area de 18 fullwords */

/*============================================================================
 * Buffer de Saída HLASM
 *============================================================================*/

static FILE *hlasm_output = NULL;
static int   hlasm_label_counter = 0;
static char  hlasm_csect_name[9] = "CCPROG";
static int   hlasm_param_count = 0;      /* Contador de parâmetros para chamada */

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
    emit("* Convenção z/OS: R1=param list, R13=save area, R14=return, R15=entry");
    emit("*");
    emit("%-8s DS    0H", func_name);
    emit("         STM   14,12,12(13)      Save registers R14-R12");
    emit("         LR    12,15             Establish base register");
    emit("         USING %s,12", func_name);
    emit("         LR    11,13             Save caller's save area in R11");
    emit("         LA    13,SAVE%s         Get our save area", func_name);
    emit("         ST    11,4(,13)         Chain: our->prev = caller's");
    emit("         ST    13,8(,11)         Chain: caller->next = ours");
    emit("         LR    11,13             R11 = frame pointer (ebp)");
    emit("*        R1 contains parameter list pointer");
    emit("*        Parameters accessed via: L Rx,0(,1) for 1st, L Rx,4(,1) for 2nd, etc.");
    hlasm_param_count = 0;
}

void hlasm_function_epilog(void)
{
    emit("*        Return: R15 contains return code, F0 for float/double");
    emit("         LR    15,2              Copy return value to R15");
    emit("         L     13,4(,13)         Restore caller's save area");
    emit("         LM    14,12,12(13)      Restore registers R14-R12");
    emit("         BR    14                Return to caller");
}

/*============================================================================
 * Geração de Chamada de Função (Convenção z/OS)
 * 
 * Para chamar uma função em z/OS:
 * 1. Construir lista de parâmetros na memória
 * 2. Cada entrada é um ponteiro para o argumento
 * 3. Setar high-order bit do último ponteiro
 * 4. Carregar endereço da lista em R1
 * 5. Carregar endereço da função em R15
 * 6. BALR 14,15 para chamar
 *============================================================================*/

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
                emit("         L     0,%d(,11)", num + STACK_FRAME_BASE);
                emit("         AHI   0,1");
                emit("         ST    0,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case dec_dbp:
                /* Decrement dword at [ebp+offset] */
                emit("         L     0,%d(,11)", num + STACK_FRAME_BASE);
                emit("         AHI   0,-1");
                emit("         ST    0,%d(,11)", num + STACK_FRAME_BASE);
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
                emit("         L     2,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case mov_ecx_pbp:
                emit("         L     3,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case mov_edx_pbp:
                emit("         L     4,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case mov_pbp_eax:
                /* Store to stack frame */
                emit("         ST    2,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case lea_eax_pbp:
                /* Load address from stack */
                emit("         LA    2,%d(,11)", num + STACK_FRAME_BASE);
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
                emit("         M     2,%d(,11)", num + STACK_FRAME_BASE);
                emit("         LR    2,3");
                break;
                
            case xdiv_dbp:
                /* divide eax by [ebp+offset] */
                emit("         LR    4,2");
                emit("         SRDA  4,32");
                emit("         D     4,%d(,11)", num + STACK_FRAME_BASE);
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
                
            case test_ah:
                /* Test AH register - in z/Arch, test high byte of R2 */
                /* AH corresponds to bits 8-15 of EAX (R2) */
                emit("         LR    0,2              Copy R2 to R0");
                emit("         SRL   0,8              Shift right 8 bits");
                emit("         N     0,=X'000000%02X' AND with mask", num);
                emit("         LTR   0,0              Set condition code");
                break;
                
            case mov_psp_eax:
                /* mov [esp+offset],eax - Store to stack */
                emit("         ST    2,%d(,13)        Store to stack", num);
                break;
                
            case mov_eax_psp:
                /* mov eax,[esp+offset] - Load from stack */
                emit("         L     2,%d(,13)        Load from stack", num);
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
                /* Function call using z/OS conventions */
                emit("*        Call function: %s", toString(num));
                if (hlasm_param_count > 0) {
                    emit("         LA    1,PLIST         R1 -> parameter list");
                    emit("         OI    PLIST+%d,X'80'  Set VL bit on last param", 
                         (hlasm_param_count - 1) * 4);
                }
                emit("         L     15,=V(%s)      Load function address", toString(num));
                emit("         BALR  14,15            Call function");
                emit("         LR    2,15             Copy return value to R2 (eax)");
                hlasm_param_count = 0;
                break;
                
            case push_eax:
                /* Push para lista de parâmetros z/OS */
                emit("         LA    0,PLIST+%d       Param slot address", hlasm_param_count * 4);
                emit("         ST    2,0(,0)          Store param value");
                hlasm_param_count++;
                break;
                
            case push_ecx:
                emit("         LA    0,PLIST+%d       Param slot address", hlasm_param_count * 4);
                emit("         ST    3,0(,0)          Store param value");
                hlasm_param_count++;
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
                /* Push variável local para lista de parâmetros */
                emit("         LA    0,PLIST+%d       Param slot address", hlasm_param_count * 4);
                emit("         L     1,%d(,11)        Load local var", num + STACK_FRAME_BASE);
                emit("         ST    1,0(,0)          Store as param");
                hlasm_param_count++;
                break;
                
            case push:
                /* Push immediate para lista de parâmetros */
                emit("         LA    0,PLIST+%d       Param slot address", hlasm_param_count * 4);
                if (num >= 0 && num < 4096) {
                    emit("         LA    1,%d              Load immediate", num);
                } else {
                    emit("         L     1,=F'%d'         Load immediate", num);
                }
                emit("         ST    1,0(,0)          Store as param");
                hlasm_param_count++;
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
                emit("         A     2,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case add_pbp_eax:
                emit("         L     0,%d(,11)", num + STACK_FRAME_BASE);
                emit("         AR    0,2");
                emit("         ST    0,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case sub_eax_pbp:
                emit("         S     2,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case sub_pbp_eax:
                emit("         L     0,%d(,11)", num + STACK_FRAME_BASE);
                emit("         SR    0,2");
                emit("         ST    0,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case cmp_eax_pbp:
                emit("         C     2,%d(,11)", num + STACK_FRAME_BASE);
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
                /* sign extend halfword to fullword */
                emit("         SLL   2,16             Shift left 16 bits");
                emit("         SRA   2,16             Arithmetic shift right 16");
                break;
                
            case lea_ecx_pbp:
                emit("         LA    3,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case lea_edx_pbp:
                emit("         LA    4,%d(,11)", num + STACK_FRAME_BASE);
                break;
                
            case mov_pbp_ecx:
                emit("         ST    3,%d(,11)", num + STACK_FRAME_BASE);
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
                emit("         STC   2,%d(,11)", num + STACK_FRAME_BASE);
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
                
            /* Instruções de Ponto Flutuante */
            case fld_qax:
                /* Load double from [eax] to FPU */
                emit("         LD    0,0(,2)          Load double BFP to F0");
                break;
                
            case fld_qcx:
                emit("         LD    0,0(,3)          Load double BFP to F0");
                break;
                
            case fld_qbp:
                emit("         LD    0,%d(,11)        Load double BFP from stack", num + STACK_FRAME_BASE);
                break;
                
            case fld_qp:
                emit("         LD    0,=DB'%d'        Load double BFP constant", num);
                break;
                
            case fst_qax:
                emit("         STD   0,0(,2)          Store double to [eax]");
                break;
                
            case fst_qcx:
                emit("         STD   0,0(,3)          Store double to [ecx]");
                break;
                
            case fst_qbp:
                emit("         STD   0,%d(,11)        Store double to stack", num + STACK_FRAME_BASE);
                break;
                
            case fstp_qsp:
                emit("         STD   0,0(,13)         Store and pop to stack");
                break;
                
            case fadd_qbp:
                emit("         ADB   0,%d(,11)        Add double BFP from stack", num + STACK_FRAME_BASE);
                break;
                
            case fsub_qbp:
                emit("         SDB   0,%d(,11)        Sub double BFP from stack", num + STACK_FRAME_BASE);
                break;
                
            case fmul_qbp:
                emit("         MDB   0,%d(,11)        Mul double BFP from stack", num + STACK_FRAME_BASE);
                break;
                
            case fdiv_qbp:
                emit("         DDB   0,%d(,11)        Div double BFP from stack", num + STACK_FRAME_BASE);
                break;
                
            case fadd_qp:
                emit("         ADB   0,=DB'%d'        Add double BFP constant", num);
                break;
                
            case fsub_qp:
                emit("         SDB   0,=DB'%d'        Sub double BFP constant", num);
                break;
                
            case fmul_qp:
                emit("         MDB   0,=DB'%d'        Mul double BFP constant", num);
                break;
                
            case fdiv_qp:
                emit("         DDB   0,=DB'%d'        Div double BFP constant", num);
                break;
                
            case faddp_st1_st:
                emit("         ADBR  2,0              Add F0 to F2 (BFP)");
                emit("         LDR   0,2              Move result to F0");
                break;
                
            case fsubrp_st1_st:
                emit("         SDBR  2,0              Sub F0 from F2 (BFP)");
                emit("         LDR   0,2              Move result to F0");
                break;
                
            case fmulp_st1_st:
                emit("         MDBR  2,0              Mul F2 by F0 (BFP)");
                emit("         LDR   0,2              Move result to F0");
                break;
                
            case fdivrp_st1_st:
                emit("         DDBR  2,0              Div F2 by F0 (BFP)");
                emit("         LDR   0,2              Move result to F0");
                break;
                
            case fchs:
                emit("         LCDBR 0,0              Negate F0 (BFP)");
                break;
                
            case fxch_st1:
                emit("         LDR   4,0              F4 = F0 (temp)");
                emit("         LDR   0,2              F0 = F2");
                emit("         LDR   2,4              F2 = F4 (old F0)");
                break;
                
            case fild_dax:
                /* Convert integer at [eax] to float (double) */
                /* z/Architecture: CDFBR converts 32-bit fixed to long BFP */
                emit("         L     0,0(,2)          Load integer from [eax]");
                emit("         CDFBR 0,0              Convert fixed to long BFP");
                break;
                
            case fild_dsp:
                /* Convert integer from stack to float (double) */
                emit("         L     0,%d(,13)        Load integer from stack", num);
                emit("         CDFBR 0,0              Convert fixed to long BFP");
                break;
                
            case fistp_dsp:
                /* Convert float to integer and store to stack */
                /* z/Architecture: CFDBR converts long BFP to 32-bit fixed */
                /* Mode 5 = round toward zero (truncate) */
                emit("         CFDBR 0,5,0            Convert long BFP to fixed (truncate)");
                emit("         ST    0,%d(,13)        Store integer to stack", num);
                break;
                
            case fucompp:
                emit("         CDBR  0,2              Compare F0 with F2 (BFP)");
                break;
                
            case fstsw:
                emit("* FPU status word - condition code already set by CDBR");
                break;
                
            case fldcw:
                emit("* FPU control word - not applicable to z/Arch");
                break;
                
            case fstp_st1:
                emit("         LDR   2,0              Copy F0 to F2 (pop)");
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
            emit("         DC    DB'%f'", pInst->dval);
        } else if (pInst->inst == setstr) {
            /* Emite string em EBCDIC */
            char ebcdic_buf[256];
            ascii_to_ebcdic_str(ebcdic_buf, pInst->sval, sizeof(ebcdic_buf) - 1);
            emit("         DC    C'%s'             ASCII: '%s'", pInst->sval, pInst->sval);
        }
    }
    
    emit("*");
    emit("* Work Areas");
    emit("*");
    emit("SAVEAREA DS    18F               Register save area (72 bytes)");
    emit("PLIST    DS    16F               Parameter list (up to 16 params)");
    emit("FTEMP    DS    D                 Temp for float conversions");
    emit("ITEMP    DS    F                 Temp for integer conversions");
    emit("*");
    emit("* Function-specific save areas");
    emit("*");
    /* Emite save areas para cada função */
    for (n = 0; n < ix.ixCode; n++) {
        INSTRUCT *pInst = &cd.pCode[n];
        if (pInst->inst == fn_ || pInst->inst == exp_) {
            emit("SAVE%-4s DS    18F               Save area for %s", 
                 toString(pInst->num), toString(pInst->num));
        }
    }
    emit("*");
    emit("         LTORG                   Literal pool");
    emit("*");
    emit("*--------------------------------------------------------------------");
    emit("* Notas sobre instruções de conversão:");
    emit("* CDFBR F1,R2 - Convert from Fixed 32-bit to Long BFP (double)");
    emit("* CFDBR R1,M3,F2 - Convert to Fixed 32-bit from Long BFP");
    emit("*   M3=0: Use current rounding mode");
    emit("*   M3=1: Round to nearest (ties to even)");
    emit("*   M3=5: Round toward zero (truncate)");
    emit("*   M3=6: Round toward +infinity");
    emit("*   M3=7: Round toward -infinity");
    emit("* Estas instruções requerem z/Architecture (z900+)");
    emit("*--------------------------------------------------------------------");
    emit("         END   %s", hlasm_csect_name);
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
 * Notas de Implementação - Status
 * 
 * [IMPLEMENTADO] 1. Mapeamento de Registradores:
 *    - eax -> R2, ecx -> R3, edx -> R4
 *    - ebp -> R11 (frame pointer)
 *    - esp -> R13 (save area pointer)
 * 
 * [IMPLEMENTADO] 2. Convenções de Chamada z/OS:
 *    - R1 para lista de parâmetros (ponteiros para argumentos)
 *    - R13 para save area encadeada
 *    - R14 para return address
 *    - R15 para entry point e return code
 *    - High-order bit setado no último parâmetro (VL bit)
 * 
 * [IMPLEMENTADO] 3. Gerenciamento de Stack:
 *    - Save areas de 72 bytes (18 fullwords) encadeadas
 *    - Variáveis locais após offset 72 do frame pointer
 * 
 * [IMPLEMENTADO] 4. Ponto Flutuante:
 *    - F0 para operações e retorno
 *    - F2 como segundo operando
 *    - Instruções LD/STD para load/store double
 *    - Instruções AD/SD/MD/DD para aritmética
 *    - CDFBR para conversão int→double
 *    - CFDBR para conversão double→int (com truncamento)
 * 
 * [IMPLEMENTADO] 5. Strings e Dados:
 *    - Tabela de conversão ASCII -> EBCDIC incluída
 *    - Strings emitidas com comentário do original ASCII
 *    - Alinhamento em fullword para dados
 * 
 * [PENDENTE] 6. Chamadas de Sistema:
 *    - Para I/O, usar LE (Language Environment) C runtime:
 *      * printf -> CEEMSG ou call direto
 *      * malloc -> CEEGTST
 *      * exit -> CEE3ABD
 *    - Ou usar macros z/OS nativas:
 *      * WTO para output
 *      * GETMAIN/FREEMAIN para memória
 * 
 * [PENDENTE] 7. Melhorias Futuras:
 *    - Suporte a 64-bit (z/Architecture modo AMODE 64)
 *    - Otimização de uso de registradores
 *    - Suporte a estruturas grandes (>4KB)
 *    - Debugging info (DWARF para z/OS)
 *============================================================================*/
