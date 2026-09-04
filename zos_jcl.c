#include "cc.h"

static void makeMemberName(const char *path, char member[9])
{
	const char *base = strrchr(path, '/');
	const char *windowsBase = strrchr(path, '\\');
	size_t length = 0U;
	if (windowsBase != NULL && (base == NULL || windowsBase > base))
	{
		base = windowsBase;
	}
	base = base == NULL ? path : base + 1;
	while (*base != '\0' && *base != '.' && length < 8U)
	{
		unsigned char c = (unsigned char)*base++;
		if (isalnum(c))
		{
			member[length++] = (char)toupper(c);
		}
	}
	if (length == 0U)
	{
		memcpy(member, "PROGRAM", 7U);
		length = 7U;
	}
	if (isdigit((unsigned char)member[0]))
	{
		member[0] = 'P';
	}
	member[length] = '\0';
}

void zos_write_jcl(const char *assemblyFile, const char *jclFile)
{
	FILE *input = fopen(assemblyFile, "r");
	FILE *output;
	char member[9];
	char line[256];
	int inputClose;
	int outputClose;
	if (input == NULL)
	{
		error("zos.jcl", "cannot read assembler file '%s'", assemblyFile);
	}
	output = fopen(jclFile, "w");
	if (output == NULL)
	{
		fclose(input);
		error("zos.jcl", "cannot create JCL file '%s'", jclFile);
	}
	makeMemberName(assemblyFile, member);
	fprintf(output,
	        "//CCJOB JOB (ACCT),'CC HLASM',CLASS=A,MSGCLASS=H,NOTIFY=&SYSUID\n"
	        "//ASM EXEC PGM=ASMA90,\n"
	        "// PARM='OBJECT,GOFF,RENT,XREF(FULL),FLAG(0)'\n"
	        "//SYSLIB DD DISP=SHR,DSN=CEE.SCEEMAC\n"
	        "//SYSUT1 DD UNIT=SYSDA,SPACE=(CYL,(1,1))\n"
	        "//SYSPRINT DD SYSOUT=*\n"
	        "//SYSLIN DD DSN=&&OBJ,DISP=(NEW,PASS),UNIT=SYSDA,\n"
	        "// SPACE=(TRK,(5,5)),DCB=(RECFM=FB,LRECL=80,BLKSIZE=0)\n"
	        "//SYSIN DD *\n");
	while (fgets(line, sizeof(line), input) != NULL)
	{
		fputs(line, output);
	}
	if (ferror(input))
	{
		error("zos.jcl", "read error in '%s'", assemblyFile);
	}
	fprintf(output,
	        "/*\n"
	        "//BIND EXEC PGM=IEWL,COND=(0,NE,ASM),\n"
	        "// PARM='MAP,XREF,LIST,NORENT,NOREUS,AMODE=31,RMODE=ANY'\n"
	        "//SYSPRINT DD SYSOUT=*\n"
	        "//SYSUT1 DD UNIT=SYSDA,SPACE=(CYL,(1,1))\n"
	        "//SYSLIB DD DISP=SHR,DSN=CEE.SCEELKED\n"
	        "//SYSLMOD DD DSN=&&LOAD(%s),DISP=(NEW,PASS),UNIT=SYSDA,\n"
	        "// SPACE=(TRK,(5,5,1)),DSNTYPE=LIBRARY\n"
	        "//SYSLIN DD DSN=&&OBJ,DISP=(OLD,DELETE)\n"
	        "// DD *\n"
	        " NAME %s(R)\n"
	        "/*\n"
	        "//RUN EXEC PGM=%s,COND=((0,NE,ASM),(0,NE,BIND)),PARM=''\n"
	        "//STEPLIB DD DSN=&&LOAD,DISP=(OLD,DELETE)\n"
	        "//SYSOUT DD SYSOUT=*\n"
	        "//SYSPRINT DD SYSOUT=*\n"
	        "//CEEDUMP DD SYSOUT=*\n"
	        "//SYSUDUMP DD SYSOUT=*\n",
	        member,
	        member,
	        member);
	inputClose = fclose(input);
	outputClose = fclose(output);
	if (inputClose != 0 || outputClose != 0)
	{
		error("zos.jcl", "cannot finish JCL file '%s'", jclFile);
	}
	printf("z/OS JCL written to: %s\n", jclFile);
}
