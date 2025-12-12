/*
 * cc.h - Header principal do compilador C
 * 
 * Contém todas as definições de tipos, estruturas e declarações
 * compartilhadas entre os módulos do compilador.
 */

#ifndef CC_H
#define CC_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Abstrações cross-platform */
#include "cc_platform.h"

/*============================================================================
 * Constantes e Enumerações
 *============================================================================*/

enum
{
	oSRC = 1, oTOKEN = 2, oASM = 4, oTRACE = 8, oDEBUG = 16,
	oDLL = 32, oLINES = 64, oUOPT = 128, oNAME = 256
};

#define OPERATOR2 "== != <= >= |= += -= *= /= %= &= ^= >> << ++ -- && || -> "

enum
{
	TK_SYMBOL = 0, TK_NAME, TK_NUMBER, TK_STRING,
	TK_CHAR, TK_SHORT, TK_INT, TK_FLOAT, TK_DOUBLE
};

#define HASHSIZE_TOKEN  9973
enum { VAL = 0, ADDR };
#define globTable	0
#define AT_TYPE		0x01000000
#define AT_USER		0x02000000
#define AT_IMPT		0x04000000
#define AT_EXPT		0x08000000
#define AT_ADDR		0x00FFFFFF

enum { ST_FUNC = 1, ST_GVAR, ST_STRUCT, ST_ENUM };
enum { AD_CODE = 1, AD_DATA, AD_ZERO, AD_STACK, AD_IMPORT, AD_CONST };

#define NM_CDECL	0x0001
#define NM_WINAPI	0x0002
#define NM_VAR		0x0004
#define NM_EXPORT	0x0008
#define NM_FUNC		(NM_CDECL | NM_WINAPI | NM_EXPORT)	
#define NM_STRUCT	0x0010
#define NM_ATTR		0x0020
#define NM_ENUM		0x0040

#define MEMALIGN  	0x1000
#define RAWALIGN  	0x0200
#define MEMALIGN1	0x0fff		// MEMALIGN - 1
#define RAWALIGN1	0x01ff		// RAWALIGN - 1
#define IMAGEBASE 	0x00400000
#define DLLBASE   	0x10000000

/*============================================================================
 * Estruturas de Dados
 *============================================================================*/

typedef struct _CMDPARAM
{
	int  nSrc;
	char *srcfile[8];
	char outfile[MAX_PATH];
	char impfiles[522];
	char MCCDIR[MAX_PATH];
} CMDPARAM;

typedef struct _HDATA
{
	char *key;
	void *val;
	int seq;
} HDATA;

typedef struct _HASH
{
	HDATA *tbl;
	int   type;
	int   size, entries;
	struct _HASH *pNext;
} HASH;

typedef struct _SRCLINE
{
	int  filenumber, linenumber;
	char *srccode;
} SRCLINE;

typedef struct _TOKEN
{
	int    type;
	int    ival;
	double dval;
	char   *token;
	int    filenumber, linenumber;
} TOKEN;

typedef struct _MCC
{
	char *srcFile[128];
	int  lines[128];
	int  totalLines;
	int  nSrcFile;
	int  mainfile;
	SRCLINE *pSrcLine;
	int  sizeSrcLine;
	int  nSrcLine;
	HASH hash;
	int  typeApp;
	int  nPreFile;
} MCC;

typedef struct _PTRS_TYPE
{
	int ptrs, type;
} PTRS_TYPE;

typedef struct _Name
{
	int type;
	int idName;
	int dataType;
	int addrType;
	int address;
	int ptrs;
	int size[8];
	PTRS_TYPE *argpt;
	int argc;
	struct _Name *pBgn, *pEnd;
	struct _Name *pNext;
} Name;

typedef struct _Block
{
	int idFunc;
	int blockDepth;
	HASH hash;
} Block;

typedef struct _INSTRUCT
{
	int  inst, num, attr, offset, size, refs;
	double dval;
	char *sval;
	int  regs;
} INSTRUCT;

typedef struct _HID
{
	int  CHAR, SHORT, INT, FLOAT, DOUBLE, HVOID;
	int  STRUCT, TYPEDEF, IF, ELSE, WHILE, DO, FOR, RETURN, CONTINUE, BREAK, SWITCH, CASE,
		DEFAULT, SIZEOF, DOTS3, HWINAPI, HCONST, EXTERN, DECLSP, DLLIMPT, DLLEXPT, STATIC, ENUM;
} HID;

typedef struct _INDEX
{
	int  tix;
	int  ixCode, ixData, ixZero, ixString;
	int  ixLoc;
} INDEX;

typedef struct _VALUE
{
	int	 mode;
	int  ptrs, type;
	int  ival;
	double  rval;
	int	 fConst, fAddr;
} VALUE;

typedef struct _CODE
{
	TOKEN *token;
	int  nToken;
	HASH hash;
	INSTRUCT *pCode;
	int  sizeCode;
	int  currTable;
	Block block[100];
	int  baseSpace;
	int  offset;
} CODE;

typedef struct _SECTION
{
	int  CodeAddr, CodeSize, DataAddr, DataSize;
	int  ImptAddr, ImptSize, ExptAddr, ExptSize, RelocAddr, RelocSize;
} SECTION;

typedef struct _DLL
{
	char *dllname;
	int  idFunc[64];
	int  nFunc;
} DLL;

typedef struct _EXE
{
	int  base, entryPoint;
	int  lenImpt, lenExpt, sizeImage;
	DLL  dll[32];
	int  nDLL, useDLL;
	int  useFunc;
	int  locs[1000], nLocs;
	int  cnt[100], nPages;
} EXE;

typedef struct _Variable
{
	int tag, type, id;
	int width;
	int pointers, arrays;
	int size[8], length;
} Variable;

/*============================================================================
 * Variáveis Globais (extern)
 *============================================================================*/

extern int      opt;
extern MCC      mcc;
extern CMDPARAM cmd;
extern HID      ID;
extern INDEX    ix;
extern CODE     cd;
extern SECTION  mem, raw;
extern EXE      exe;
extern Variable var;

/*============================================================================
 * Funções - util.c
 *============================================================================*/

void  error(const char *loc, const char *format, ...);
int   htoi(int c);
int   esc_char(char *p);
int   isAlpha(int c);
int   isAlNum(int c);
int   isKanji(int c);
char *endOfQuote(char *p);
void *xcheck(void *p);
void *xalloc(int size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(char *q);
int   xstrcpy(char *p, char *q);
int   xstrlen(char *p);

/* Hash table functions */
void  initHash(int type, int size, HASH *pHash);
void  reallocHash(int newSize, HASH *pHash);
void  freeHash(HASH *pHash);
int   hash(char *key, int size);
int   put(char *key, void *val, HASH *pHash);
void *get(char *key, HASH *pHash);
void  printHash(HASH *pHash);

/*============================================================================
 * Funções - prepro.c
 *============================================================================*/

void prepro(char *srcfile);
void initPrepro(void);
void addStartup(void);

/*============================================================================
 * Funções - lexer.c
 *============================================================================*/

void lex(void);
void printToken(int n);
void setToken(TOKEN *token, HASH *pHash);

/*============================================================================
 * Funções - parser.c
 *============================================================================*/

void parse(void);
void init(void);
void program(void);
void functionDefinition(void);
void variableDeclaration(Name *pStruct, int status);
void structDeclaration(void);
void enumDefinition(void);
void typeSpecifier(void);
int  isTypeSpecifier(int ix);
int  varDeclarator(int fParam);
void initializer(Name *name, int atype, int depth, int addr);
int  align(int pos, int width);
void parameterDeclaration(void);
int  compoundStatement(int locBreak, int locContinue, int sizeRet);
void statement(int locBreak, int locContinue, int sizeRet);

/* Helper functions */
char *toStr(int tix);
char *toString(int id);
int   id(char* str);
int   id2(char* op);
int   isId(int n);
int   is(int id);
int   isN(int id, int n);
int   is2(char *op);
int   ispp(int id);
int   is2pp(char *op);
void  skip(int id);
int   sizeOfDataType(int type);
void  constExpression(VALUE *pv);
int   constIntExpression(void);

/* Name table functions */
void  createNameTable(int idFunc);
void  deleteNameTable(void);
Name *newName(int type, int dataType, int name, int addrType, int address);
Name *putName(int ixBlk, Name *pName);
Name *appendName(int ixBlk, int type, int dataType, int name, int addrType, int address);
Name *getNameFromTable(int ixBlk, int type, int name);
Name *getNameFromAllTable(int ixBlk, int type, int name);
Name *getStruct(int type);
int   getPtr(int type);
Name *getAttr(int type, int name);
Name *getMember(int type, int n);
void  printNameTable(int ixBlk);

/*============================================================================
 * Funções - expr.c
 *============================================================================*/

void expression(int mode, VALUE *pv);
void assignExpression(int mode, VALUE *pv);
void conditionalExpression(int mode, VALUE *pv);
void logicalOrExpression(int mode, VALUE *pv);
void logicalAndExpression(int mode, VALUE *pv);
void orExpression(int mode, VALUE *pv);
void xorExpression(int mode, VALUE *pv);
void andExpression(int mode, VALUE *pv);
void equalityExpression(int mode, VALUE *pv);
void relationalExpression(int mode, VALUE *pv);
void shiftExpression(int mode, VALUE *pv);
void addExpression(int mode, VALUE *pv);
void mulExpression(int mode, VALUE *pv);
void castExpression(int mode, VALUE *pv);
void unaryExpression(int mode, VALUE *pv);
void primaryExpression(int mode, VALUE *pv);
void infixOperation(int op, VALUE *v1, VALUE *v2, int left_bgn, int left_end);
void setValue(int mode, int ptrs, int type, VALUE *pv);

/*============================================================================
 * Funções - codegen.c (Interface de geração de código)
 *============================================================================*/

/* Funções de emissão de código intermediário */
void reallocCode(int size);
void outCode3(int code, int num, int attr);
void outCode2(int code, int num);
void outCode1(int code);
void delCodes(int from, int to);
void delCode(int n);

/* Funções de controle de fluxo */
void jumpFalse(int loc, char *msg);
void jumpTrue(int loc, char *msg);
int  loc(void);

/* Funções de dados */
void outData(int inst, int offset);
void outInteger(int offset, int size, int ival);
void outReal(int offset, double dval);
int  outString(int offset, char *sval);
void outDataChar(int c);
void outDataShort(int n);
void outDataInt(int n);
void outDataAddr(int p);
void outDataDouble(double d);

/* Funções de load/store */
void loadAddr(int type, int addr);
void loadValue(int type, int fPtr);
void setFpuStack2(int type1, int type2);
void incdec(int type, int fPtr, int fI, int reg);

/* Funções de expressão auxiliares */
void expr(int mode);
void expr2(int mode);

/*============================================================================
 * Funções - backend_x86.c (Backend específico x86/PE)
 *============================================================================*/

/* Estrutura de instrução x86 */
typedef struct _INSTRUCTION
{
	int  opcode;
	char *hexcode, *hexcode2, *mnemonic;
	int  regs_size;
} INSTRUCTION;

extern INSTRUCTION instruction[];

void initInstruction(void);

#ifdef PLATFORM_WINDOWS
/* Funções de DLL/Import/Export (apenas Windows) */
void  initDLL(char *dllnames);
int   importDLL(void);
BYTE *getImport(void);
int   initExport(void);
BYTE *getExport(void);
void  initReloc(void);
BYTE *getReloc(void);
void  setDLL(int ixDLL, int id);

/* Linker x86/PE */
void link(void);
void writeExe(BYTE *bufImport, BYTE *bufExport, BYTE *CodeBuffer,
	int lenCode, BYTE *DataBuffer, int lenData);
#endif /* PLATFORM_WINDOWS */

/*============================================================================
 * Funções - backend_hlasm.c (Backend HLASM para Mainframe)
 *============================================================================*/

#ifdef USE_HLASM_BACKEND
void hlasm_link(const char *output_file);
#endif

/*============================================================================
 * Constantes de Registradores (usadas na otimização de código)
 *============================================================================*/

enum { A = 0x0100, C = 0x0200, D = 0x0400, AC = 0x0300, AD = 0x0500, CD = 0x0600, ACD = 0x0700, X = 0x0700 };

/*============================================================================
 * Opcodes x86 (enumeração)
 *============================================================================*/

enum
{
	push = 1, push_eax, push_ecx, push_pbp, pop_eax, pop_ecx, pop_edx, inc_dbp, dec_dbp,
	add_eax_ecx, add_eax_edx, add_eax, add_ecx, add_pcx_eax, add_pcx_ax, add_pcx_al, add_esp,
	add_dax, add_dcx, add_ddx, add_bax, add_bcx, add_bdx, add_eax_pbp, add_ecx_pbp,
	add_pbp_eax, add_pbp_ecx, sub_pbp_ecx, sub_pbp_eax, and_pbp_eax, or_pbp_eax, xor_pbp_eax,
	sub_eax, sub_eax_pbp, sub_eax_ecx, sub_pcx_eax, sub_pcx_ax, sub_pcx_al, sub_esp, sub_dax,
	sub_dcx, sub_ddx, sub_bax, sub_bcx, sub_bdx, imul_eax_ecx, imul_eax_eax, imul_edx_edx,
	imul_eax_pbp, xdiv_dbp, cmp_eax_ecx, cmp_ecx_eax, cmp_eax, cmp_eax_pbp, cmp_ah, test_ah,
	test_eax_eax, and_eax_ecx, and_pcx_eax, and_pcx_ax, and_pcx_al, and_ah, or_eax_ecx,
	or_pcx_eax, or_pcx_ax, or_pcx_al, xor_eax_eax, xor_pcx_eax, xor_pcx_ax, xor_pcx_al,
	xor_eax_ecx, xor_ah, shl_eax, shl_edx, shl_eax_cl, shr_eax_cl, neg_eax, mov_ecx_eax,
	mov_edx_eax, mov_eax_edx, mov_eax, mov_ecx, mov_dax, mov_wax, mov_bax, mov_eax_pax,
	mov_eax_pcx, mov_pcx_eax, mov_pcx_ax, mov_pcx_al, mov_pax_ecx, mov_pax_cx, mov_pax_cl,
	mov_eax_pbp, mov_ecx_pbp, mov_pbp_ecx, mov_ecx_pax, mov_edx_pax, mov_edx_pbp, mov_pbp_eax,
	mov_pbp_al, mov_psp_eax, mov_eax_psp, movsx_eax_wax, movsx_eax_bax, movsx_ecx_wcx,
	movsx_ecx_bcx, lea_eax_pbp, lea_ecx_pbp, lea_edx_pbp, mov_eax_ad1, mov_eax_ad2,
	mov_eax_ad4, mov_eax_ad8, mov_eax_da1, mov_eax_da2, mov_eax_da4, mov_eax_da8, lea_eax_ad1,
	lea_eax_ad2, lea_eax_ad4,
	lea_eax_ad8, lea_eax_da1, lea_eax_da2, lea_eax_da4, lea_eax_da8, lea_ecx_da1, lea_ecx_da2,
	lea_ecx_da4, lea_ecx_da8, xchg_eax_ecx, cwde, jmp, jz, jnz, jl, jge, jle, jg, call,
	xent, xret, sete_eax, setne_eax, setl_eax, setge_eax, setle_eax, setg_eax, fchs, fxch_st1,
	fld_qax, fld_qcx, fld_qbp, fld_qp, fldcw, fstp_qsp, fstsw, fst_qax, fst_qcx, fst_qbp,
	fadd_qbp, fadd_qp, fsub_qbp, fsub_qp, fmul_qbp, fmul_qp, fdiv_qbp, fdiv_qp, fstp_st1,
	faddp_st1_st, fsubrp_st1_st, fmulp_st1_st, fdivrp_st1_st, fucompp, fistp_dsp, fild_dax,
	fild_dsp, xdiv_ecx, xmod_ecx, setint, setreal, setstr, fn_, exp_, loc_
};

#endif /* CC_H */
