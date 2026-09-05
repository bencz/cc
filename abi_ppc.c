/* ELF System V and AIX XCOFF linkage; no instruction selection lives here. */
#include "cc.h"
#include "ppc.h"

static void classifySysv(PpcArgumentCursor *cursor, IrType type, PpcArgumentLocation *location)
{
	location->generalRegister = -1;
	location->floatingRegister = -1;
	location->stackOffset = -1;
	location->words = type.bits > 32U ? 2 : 1;
	if (type.kind == IR_TYPE_FLOAT && cursor->floating < 8)
	{
		location->floatingRegister = ++cursor->floating;
	}
	else if (type.kind != IR_TYPE_FLOAT && cursor->words < 8)
	{
		location->generalRegister = 3 + cursor->words++;
	}
	else
	{
		int alignment = location->words * 4;
		cursor->stack = (cursor->stack + alignment - 1) & -alignment;
		location->stackOffset = 8 + cursor->stack;
		cursor->stack += alignment;
	}
}

static void classifyAix(PpcArgumentCursor *cursor, IrType type, PpcArgumentLocation *location)
{
	location->words = type.bits > 32U ? 2 : 1;
	location->generalRegister = cursor->words < 8 ? 3 + cursor->words : -1;
	location->floatingRegister = -1;
	location->stackOffset = 24 + cursor->words * 4;
	if (type.kind == IR_TYPE_FLOAT && cursor->floating < 13)
	{
		location->floatingRegister = ++cursor->floating;
	}
	/* The word image also carries variadic arguments and may straddle r10. */
	cursor->words += location->words;
	cursor->stack = cursor->words * 4;
}

static void textSysv(FILE *output)
{
	fputs("\t.text\n", output);
}

static void textAix(FILE *output)
{
	fputs("\t.csect .text[PR],2\n", output);
}

static void entrySysv(FILE *output, const char *name, int internal)
{
	textSysv(output);
	if (!internal)
	{
		fprintf(output, "\t.globl %s\n", name);
	}
	fprintf(output, "\t.p2align 2\n\t.type %s,@function\n%s:\n", name, name);
}

static void entryAix(FILE *output, const char *name, int internal)
{
	fprintf(output,
	        "\t.%s %s[DS]\n\t.%s .%s\n",
	        internal ? "lglobl" : "globl",
	        name,
	        internal ? "lglobl" : "globl",
	        name);
	fprintf(output, "\t.csect %s[DS],2\n\t.long .%s,TOC[TC0],0\n", name, name);
	textAix(output);
	fprintf(output, "\t.align 2\n.%s:\n", name);
}

static void endSysv(FILE *output, const char *name)
{
	fprintf(output, "\t.size %s,.-%s\n", name, name);
}

static void endAix(FILE *output, const char *name)
{
	(void)name;
	/* Mandatory traceback header, no optional fields or nonvolatile saves. */
	fputs("\t.long 0\n\t.byte 0,0,0,1,128,0,0,0\n", output);
}

static void addressSysv(FILE *output, int reg, const char *name, unsigned int serial)
{
	(void)serial;
	fprintf(output, "\tlis %d,%s@ha\n\taddi %d,%d,%s@l\n", reg, name, reg, reg, name);
}

static void addressAix(FILE *output, int reg, const char *name, unsigned int serial)
{
	fprintf(output, "\t.toc\nL..CCtoc%u:\n\t.tc L..CCentry%u[TC],%s\n", serial, serial, name);
	textAix(output);
	fprintf(output, "\tlwz %d,L..CCtoc%u(2)\n", reg, serial);
}

static void callSysv(FILE *output, const char *name)
{
	fprintf(output, "\tbl %s\n", name);
}

static void callAix(FILE *output, const char *name)
{
	fprintf(output, "\tbl .%s[PR]\n\tnop\n", name);
}

static void indirectSysv(FILE *output)
{
	fputs("\tmtctr 12\n\tbctrl\n", output);
}

static void indirectAix(FILE *output)
{
	fputs("\tstw 2,20(1)\n\tlwz 0,0(12)\n\tlwz 2,4(12)\n"
	      "\tlwz 11,8(12)\n\tmtctr 0\n\tbctrl\n\tlwz 2,20(1)\n",
	      output);
}

const PpcAbi ppcSysvAbi = {"System V ELF32",
                           8,
                           0,
                           4,
                           8,
                           0,
                           classifySysv,
                           textSysv,
                           entrySysv,
                           endSysv,
                           addressSysv,
                           callSysv,
                           indirectSysv};

const PpcAbi ppcAixAbi = {"AIX XCOFF32",
                          24,
                          32,
                          8,
                          13,
                          1,
                          classifyAix,
                          textAix,
                          entryAix,
                          endAix,
                          addressAix,
                          callAix,
                          indirectAix};
