/* Shared compiler types, constants, state, and module interfaces. */

#ifndef CC_H
#define CC_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef long
#undef long
#endif
#include <stdint.h>
#include <limits.h>
#include <time.h>

/* Platform abstraction */
#include "cc_platform.h"
#include "target.h"
#include "ir.h"

#if defined(_MSC_VER)
#define CC_NORETURN __declspec(noreturn)
#elif defined(__GNUC__)
#define CC_NORETURN __attribute__((noreturn))
#else
#define CC_NORETURN
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* Constants and enumerations */

enum
{
	oSRC = 1,
	oTOKEN = 2,
	oASM = 4,
	oTRACE = 8,
	oDEBUG = 16,
	oDLL = 32,
	oLINES = 64,
	oUOPT = 128,
	oNAME = 256
};

#define OPERATOR2 "== != <= >= |= += -= *= /= %= &= ^= >> << ++ -- && || -> "

typedef enum _FrontendOperator
{
	FRONTEND_OP_SHIFT_LEFT = -2,
	FRONTEND_OP_SHIFT_RIGHT = -3
} FrontendOperator;

enum
{
	TK_SYMBOL = 0,
	TK_NAME,
	TK_NUMBER,
	TK_STRING,
	TK_CHAR,
	TK_SHORT,
	TK_INT,
	TK_UINT,
	TK_LONG,
	TK_ULONG,
	TK_FLOAT,
	TK_DOUBLE
};

#define HASHSIZE_TOKEN 9973

enum
{
	VAL = 0,
	ADDR
};

#define globTable 0
#define AT_TYPE 0x01000000
#define AT_USER 0x02000000
#define AT_IMPT 0x04000000
#define AT_EXPT 0x08000000
#define AT_ADDR 0x00FFFFFF
#define AT_AMBIGUOUS_IMPORT AT_ADDR

enum
{
	ST_FUNC = 1,
	ST_GVAR,
	ST_STRUCT,
	ST_ENUM
};

enum
{
	AD_CODE = 1,
	AD_DATA,
	AD_ZERO,
	AD_STACK,
	AD_IMPORT,
	AD_CONST
};

#define NM_CDECL 0x0001
#define NM_WINAPI 0x0002
#define NM_VAR 0x0004
#define NM_EXPORT 0x0008
#define NM_FUNC (NM_CDECL | NM_WINAPI | NM_EXPORT)
#define NM_STRUCT 0x0010
#define NM_ATTR 0x0020
#define NM_ENUM 0x0040
#define NM_STATIC 0x0080
#define NM_TYPEDEF 0x0100
#define NM_UNION 0x0200

#define MEMALIGN 0x1000
#define RAWALIGN 0x0200
#define MEMALIGN1 0x0fff // MEMALIGN - 1
#define RAWALIGN1 0x01ff // RAWALIGN - 1
#define IMAGEBASE 0x00400000
#define DLLBASE 0x10000000

/* Data structures */

typedef struct _CMDPARAM
{
	const TargetDescriptor *target;
	ExecutionCharset executionCharset;
	int warningsAsErrors;
	int nSrc;
	char *srcfile[32];
	char outfile[MAX_PATH];
	char irfile[MAX_PATH];
	char jclfile[MAX_PATH];
	char impfiles[522];
	char MCCDIR[MAX_PATH];
	char peSysroot[MAX_PATH];
	char **libraryPaths;
	int libraryPathCount;
	int libraryPathCapacity;
	int nDefines;
	char *defines[64];
} CMDPARAM;

typedef struct _HDATA
{
	char *key;
	void *val;
	int seq;
	unsigned char state;
} HDATA;

typedef struct _HASH
{
	HDATA *tbl;
	int type;
	int size, entries;
	int nextSequence;
	struct _HASH *pNext;
} HASH;

typedef struct _SRCLINE
{
	int filenumber, linenumber;
	char *srccode;
} SRCLINE;

typedef struct _TOKEN
{
	int type;
	int ival;
	int numericEscape;
	double dval;
	char *token;
	int filenumber, linenumber;
} TOKEN;

typedef struct _MCC
{
	char *srcFile[128];
	int lines[128];
	int totalLines;
	int nSrcFile;
	int mainfile;
	SRCLINE *pSrcLine;
	int sizeSrcLine;
	int nSrcLine;
	HASH hash;
	int typeApp;
	int nPreFile;
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
	int arrays;
	int size[8];
	int alignment;
	IrLocalId irLocal;
	IrSymbolId irSymbol;
	PTRS_TYPE *argpt;
	int argc;
	int isFunctionPointer;
	int returnPointers;
	int functionCallConvention;
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
	int inst, num, attr, offset, size, refs;
	double dval;
	char *sval;
	int regs;
} INSTRUCT;

typedef struct _HID
{
	int T_BOOL, T_CHAR, T_SCHAR, T_UCHAR, T_SHORT, T_USHORT, T_INT, T_UINT, T_LONG, T_ULONG,
	    T_FLOAT, T_DOUBLE, T_VOID;
	int STRUCT, UNION, TYPEDEF, IF, ELSE, WHILE, DO, FOR, RETURN, CONTINUE, BREAK, SWITCH, CASE,
	    DEFAULT, SIZEOF, DOTS3, HWINAPI, HCONST, EXTERN, DECLSP, DLLIMPT, DLLEXPT, STATIC, ENUM;
	int SIGNED, UNSIGNED, HLONG;
} HID;

typedef struct _INDEX
{
	int tix;
	int ixCode, ixData, ixZero, ixString;
	int ixLoc;
} INDEX;

typedef struct _VALUE
{
	int mode;
	int ptrs, type;
	int ival;
	double rval;
	int fConst, fAddr, objectSize;
	IrValueId irValue;
	IrValueId irAddress;
	IrSymbolId constantSymbol;
	int constantOffset;
	const Name *callable;
	Name *object;
	int arrayDepth;
} VALUE;

typedef struct _CODE
{
	TOKEN *token;
	int nToken;
	HASH hash;
	INSTRUCT *pCode;
	int sizeCode;
	int currTable;
	Block block[100];
	int baseSpace;
	int offset;
} CODE;

typedef struct _SECTION
{
	int CodeAddr, CodeSize, DataAddr, DataSize;
	int ImptAddr, ImptSize, ExptAddr, ExptSize, RelocAddr, RelocSize;
} SECTION;

typedef struct _CC_DLL
{
	char *dllname;
	int idFunc[64];
	int nFunc;
} DLL;

typedef struct _EXE
{
	int base, entryPoint;
	int lenImpt, lenExpt, sizeImage;
	DLL dll[32];
	int nDLL, useDLL;
	int useFunc;
	int *locs, nLocs, locCapacity;
	int *cnt, nPages;
} EXE;

typedef struct _Variable
{
	int tag, type, id;
	int width;
	int alignment;
	int pointers, arrays;
	int size[8], length;
	int isFunctionPointer;
	int returnPointers;
	int functionCallConvention;
	int functionParameterCount;
	PTRS_TYPE functionParameters[64];
} Variable;

/* Shared compiler state */

struct _CompilerContext
{
	int options;
	MCC sources;
	CMDPARAM command;
	HID identifiers;
	INDEX index;
	CODE code;
	SECTION memorySections;
	SECTION fileSections;
	EXE executable;
	Variable declaration;
	IrModule ir;
	IrBuilder irBuilder;
};

extern CompilerContext compiler;

/* util.c */

CC_NORETURN void error(const char *loc, const char *format, ...);
int hexDigitValue(int c);
int decodeEscape(char *p);
int decodeEscapeSequence(const char **cursor);
int isIdentifierStart(int c);
int isIdentifierContinue(int c);
int isShiftJisLeadByte(int c);
char *skipQuotedLiteral(char *p);
void *xalloc(size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *q);
int decodeString(char *p, char *q);
int decodedStringLength(char *p);

/* Hash tables */
void hashInit(int type, int size, HASH *table);
void hashFree(HASH *table);
int hashPut(char *key, void *value, HASH *table);
void *hashGet(char *key, HASH *table);
int hashRemove(char *key, HASH *table);

/* prepro.c */

void prepro(char *srcfile);
void initPrepro(void);
void addStartup(void);
void preproAddSyntheticLine(const char *source, int line);
int preproEvalExpression(const char *text, HASH *macros);
int preproExpandFunctionExpression(const char *name, const char **cursor, char **replacement);

/* lexer.c */

void lex(void);
void printToken(int n);
void setToken(TOKEN *token, HASH *pHash);

/* parser.c */

void parse(void);
void typeSpecifier(void);
int isTypeSpecifier(int ix);

/* Parser helpers */
char *toString(int id);
int id(char *str);
int id2(char *op);
int is(int id);
int isN(int id, int n);
int is2(char *op);
int ispp(int id);
int is2pp(char *op);
void skip(int id);
int sizeOfDataType(int type);
int isFloatingType(int type);
int arithmeticType(int left, int right);
int sizeOfPointer(void);
int sizeOfObjectType(int type, int pointers);
int alignmentOfObjectType(int type, int pointers);
int canonicalType(int type);
int isIntegerType(int type);
int isUnsignedType(int type);
int integerPromotion(int type);
int usualIntegerType(int leftType, int rightType);
IrType irTypeForCObject(int type, int pointers);

/* Name tables */
Name *appendName(int ixBlk, int type, int dataType, int name, int addrType, int address);
Name *getNameFromTable(int ixBlk, int type, int name);
Name *getNameFromAllTable(int ixBlk, int type, int name);
int getPtr(int type);
Name *getAttr(int type, int name);
void printNameTable(int ixBlk);

/* expr.c */

void expression(int mode, VALUE *pv);
void assignExpression(int mode, VALUE *pv);
void conditionalExpression(int mode, VALUE *pv);
void infixOperation(int op, VALUE *v1, VALUE *v2, int left_bgn, int left_end);
void setValue(int mode, int ptrs, int type, VALUE *pv);

/* semantic_ir.c */
int semanticIrActive(void);
void semanticIrConstantInteger(VALUE *value);
void semanticIrConstantFloat(VALUE *value);
void semanticIrString(VALUE *value, const char *literal);
IrSymbolId semanticIrCreateStringGlobal(const char *literal);
unsigned char *semanticIrDecodeExecutionString(const char *literal, size_t *size);
void semanticIrNameAddress(VALUE *value, const Name *name);
void semanticIrLocalAddress(VALUE *value, IrLocalId local, int offset);
void semanticIrLoad(VALUE *value);
void semanticIrOffsetAddress(VALUE *base, VALUE *index, int scale);
void semanticIrAddAddressOffset(VALUE *value, int offset);
void semanticIrBinary(int operation, VALUE *left, VALUE *right);
void semanticIrUnary(int operation, VALUE *value);
void semanticIrIncrement(VALUE *value, int increment, int postfix);
void semanticIrCast(VALUE *value, int targetType, int targetPointers);
IrValueId semanticIrCondition(VALUE *value);
IrValueId semanticIrArgument(VALUE *value, int targetType, int targetPointers);
void semanticIrCall(VALUE *value,
                    const Name *function,
                    const IrValueId *arguments,
                    int argumentCount);
void semanticIrCallIndirect(VALUE *value,
                            IrValueId callee,
                            const Name *signature,
                            const IrValueId *arguments,
                            int argumentCount);
void semanticIrReturn(VALUE *value);
IrBlockId semanticIrCreateBlock(void);
void semanticIrSelectBlock(IrBlockId block);
void semanticIrBranch(IrBlockId destination);
void semanticIrBranchCondition(VALUE *condition, IrBlockId trueBlock, IrBlockId falseBlock);
IrLocalId semanticIrCreateTemporary(int type, int pointers);
void semanticIrStoreTemporary(IrLocalId local, int type, int pointers, VALUE *value);
void semanticIrLoadTemporary(IrLocalId local, int type, int pointers, VALUE *value);
void semanticIrZeroMemory(IrLocalId local, int offset, int size);

/* codegen.c */

/* IR emission */
void reallocCode(int size);
void outCode3(int code, int num, int attr);
void outCode2(int code, int num);
void outCode1(int code);
void delCodes(int from, int to);
void delCode(int n);

/* Control flow */
void jumpFalse(int location);
void jumpTrue(int location);
int loc(void);

/* Static data */
int outString(int offset, char *sval);
void outDataChar(int c);
void outDataShort(int n);
void outDataInt(int n);
void outDataAddr(int p);
void outDataDouble(double d);

/* Loads and stores */
void loadAddr(int type, int addr);
void loadValue(int type, int fPtr);
void setFpuStack2(int type1, int type2);
void incdec(int type, int ptrs, int fIncrement, int reg);

/* Expression helpers */
void expr(int mode);
void expr2(int mode);

/* backend_x86.c */

/* x86 instruction metadata */
typedef struct _INSTRUCTION
{
	int opcode;
	char *hexcode, *hexcode2, *mnemonic;
	int regs_size;
} INSTRUCTION;

extern INSTRUCTION x86Instructions[];

void initInstruction(void);
int instructionRegisters(int opcode);
void x86LowerIr(void);
void link(void);

/* pe_exports.c */
void peLoadExportSymbols(const char *path,
                         const char *requestedName,
                         int libraryIndex,
                         char *moduleName,
                         size_t moduleNameCapacity);

/* z/OS output */

void hlasm_link(const char *output_file);
const char *zosRuntimeSource(void);
void zos_write_jcl(const char *assembly_file, const char *jcl_file);
unsigned char hlasmExecutionByte(unsigned char ascii);

/* Register masks used by code optimization */

enum
{
	A = 0x0100,
	C = 0x0200,
	D = 0x0400,
	AC = 0x0300,
	AD = 0x0500,
	CD = 0x0600,
	ACD = 0x0700,
	X = 0x0700
};

/* IR and x86 opcodes */

enum
{
	push = 1,
	push_eax,
	push_ecx,
	push_pbp,
	pop_eax,
	pop_ecx,
	pop_edx,
	inc_dbp,
	dec_dbp,
	add_eax_ecx,
	add_eax_edx,
	add_eax,
	add_ecx,
	add_pcx_eax,
	add_pcx_ax,
	add_pcx_al,
	add_esp,
	add_dax,
	add_dcx,
	add_ddx,
	add_wax,
	add_wdx,
	add_bax,
	add_bcx,
	add_bdx,
	add_eax_pbp,
	add_ecx_pbp,
	add_pbp_eax,
	add_pbp_ecx,
	sub_pbp_ecx,
	sub_pbp_eax,
	and_pbp_eax,
	or_pbp_eax,
	xor_pbp_eax,
	sub_eax,
	sub_eax_pbp,
	sub_eax_ecx,
	sub_pcx_eax,
	sub_pcx_ax,
	sub_pcx_al,
	sub_esp,
	sub_dax,
	sub_dcx,
	sub_ddx,
	sub_wax,
	sub_wdx,
	sub_bax,
	sub_bcx,
	sub_bdx,
	imul_eax_ecx,
	imul_eax_eax,
	imul_edx_edx,
	imul_eax_pbp,
	xdiv_dbp,
	cmp_eax_ecx,
	cmp_ecx_eax,
	cmp_eax,
	cmp_eax_pbp,
	cmp_ah,
	test_ah,
	test_eax_eax,
	and_eax_ecx,
	and_pcx_eax,
	and_pcx_ax,
	and_pcx_al,
	and_ah,
	or_eax_ecx,
	or_pcx_eax,
	or_pcx_ax,
	or_pcx_al,
	xor_eax_eax,
	xor_pcx_eax,
	xor_pcx_ax,
	xor_pcx_al,
	xor_eax_ecx,
	xor_ah,
	shl_eax,
	shl_edx,
	sar_eax,
	shr_eax,
	shl_eax_cl,
	sar_eax_cl,
	shr_eax_cl,
	neg_eax,
	not_eax,
	mov_ecx_eax,
	mov_edx_eax,
	mov_eax_edx,
	mov_eax,
	mov_ecx,
	mov_dax,
	mov_wax,
	mov_bax,
	mov_eax_pax,
	mov_eax_pcx,
	mov_pcx_eax,
	mov_pcx_ax,
	mov_pcx_al,
	mov_pax_ecx,
	mov_pax_cx,
	mov_pax_cl,
	mov_eax_pbp,
	mov_ecx_pbp,
	mov_pbp_ecx,
	mov_ecx_pax,
	mov_edx_pax,
	mov_edx_pbp,
	mov_pbp_eax,
	mov_pbp_al,
	mov_psp_eax,
	mov_eax_psp,
	movsx_eax_wax,
	movsx_eax_bax,
	movzx_eax_wax,
	movzx_eax_bax,
	movsx_ecx_wcx,
	movsx_ecx_bcx,
	lea_eax_pbp,
	lea_ecx_pbp,
	lea_edx_pbp,
	mov_eax_ad1,
	mov_eax_ad2,
	mov_eax_ad4,
	mov_eax_ad8,
	mov_eax_da1,
	mov_eax_da2,
	mov_eax_da4,
	mov_eax_da8,
	lea_eax_ad1,
	lea_eax_ad2,
	lea_eax_ad4,
	lea_eax_ad8,
	lea_eax_da1,
	lea_eax_da2,
	lea_eax_da4,
	lea_eax_da8,
	lea_ecx_da1,
	lea_ecx_da2,
	lea_ecx_da4,
	lea_ecx_da8,
	xchg_eax_ecx,
	cwde,
	jmp,
	jz,
	jnz,
	jl,
	jge,
	jle,
	jg,
	call,
	call_eax,
	xent,
	xret,
	sete_eax,
	setne_eax,
	setl_eax,
	setge_eax,
	setle_eax,
	setg_eax,
	setb_eax,
	setae_eax,
	setbe_eax,
	seta_eax,
	ucmp_eax_ecx,
	ucmp_ecx_eax,
	ucmp_eax,
	ucmp_eax_pbp,
	fchs,
	fxch_st1,
	fld_qax,
	fld_qcx,
	fld_qbp,
	fld_qp,
	fld_sax,
	fld_sbp,
	fstp_sbp,
	fstp_scx,
	fstp_qcx,
	fstp_ssp,
	fldcw,
	fstp_qsp,
	fstsw,
	fst_qax,
	fst_qcx,
	fst_qbp,
	fstp_qbp,
	fadd_qbp,
	fadd_qp,
	fsub_qbp,
	fsub_qp,
	fmul_qbp,
	fmul_qp,
	fdiv_qbp,
	fdiv_qp,
	fstp_st1,
	faddp_st1_st,
	fsubrp_st1_st,
	fmulp_st1_st,
	fdivrp_st1_st,
	fucompp,
	fistp_dsp,
	fistp_ueax,
	fild_dax,
	fild_dsp,
	fild_uax,
	fild_udsp,
	xdiv_ecx,
	xmod_ecx,
	udiv_ecx,
	umod_ecx,
	setint,
	setreal,
	setstr,
	setaddr,
	fn_,
	exp_,
	loc_
};

/* Compatibility aliases while modules are converted to explicit context parameters. */
#define opt (compiler.options)
#define mcc (compiler.sources)
#define cmd (compiler.command)
#define ID (compiler.identifiers)
#define ix (compiler.index)
#define cd (compiler.code)
#define mem (compiler.memorySections)
#define raw (compiler.fileSections)
#define exe (compiler.executable)
#define var (compiler.declaration)

#endif /* CC_H */
