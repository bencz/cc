/*
 * backend_x86.c - Backend x86/PE para Windows
 * 
 * Este módulo contém a tabela de instruções x86, funções de linkagem
 * e geração de executáveis PE (EXE/DLL) para Windows.
 * 
 * Para adicionar suporte a HLASM, crie um arquivo backend_hlasm.c
 * que implemente as mesmas interfaces mas gerando código assembler
 * para mainframe.
 */

#include "cc.h"

/* A tabela de instruções é necessária em todas as plataformas para o parser */

/*============================================================================
 * Tabela de Instruções x86
 *============================================================================*/

#define Z ((void*)0)

INSTRUCTION instruction[] = {
	{ push, "68", Z, "push %d", 4 },
	{ push_pbp, "FFB5", "FF75", "push [ebp%+d]", 5 },
	{ push_eax, "50", Z, "push eax", A },
	{ push_ecx, "51", Z, "push ecx", C },
	{ pop_eax, "58", Z, "pop eax", A },
	{ pop_ecx, "59", Z, "pop ecx", C },
	{ pop_edx, "5A", Z, "pop edx", D },
	{ inc_dbp, "FF85", "FF45", "inc dp[ebp%+d]", 5 },
	{ dec_dbp, "FF8D", "FF4D", "dec dp[ebp%+d]", 5 },
	{ add_eax_ecx, "03C1", Z, "add eax,ecx", AC },
	{ add_eax_edx, "03C2", Z, "add eax,edx", AD },
	{ add_eax, "05", "83C0", "add eax,%d", 5 | A },
	{ add_ecx, "81C1", "83C1", "add ecx,%d", 5 | C },
	{ add_pcx_eax, "0101", Z, "add [ecx],eax", AC },
	{ add_pcx_ax, "660101", Z, "add [ecx],ax", AC },
	{ add_pcx_al, "0001", Z, "add [ecx],al", AC },
	{ add_esp, "81C4", "83C4", "add esp,%d", 5 },
	{ add_bax, "8000", Z, "add bp[eax]", A },
	{ add_bcx, "8001", Z, "add bp[ecx]", C },
	{ add_bdx, "8002", Z, "add bp[edx]", D },
	{ add_dax, "8100", "8300", "add dp[eax],%d", 5 | A },
	{ add_dcx, "8101", "8301", "add dp[ecx],%d", 5 | C },
	{ add_ddx, "8102", "8302", "add dp[edx],%d", 5 | D },
	{ add_eax_pbp, "0385", "0345", "add eax,[ebp%+d]", 5 | A },
	{ add_ecx_pbp, "038D", "034D", "add ecx,[ebp%+d]", 5 | C },
	{ add_pbp_eax, "0185", "0145", "add [ebp%+d],eax", 5 | A },
	{ add_pbp_ecx, "018D", "014D", "add [ebp%+d],ecx", 5 | C },
	{ sub_pbp_eax, "2985", "2945", "sub [ebp%+d],eax", 5 | A },
	{ or_pbp_eax, "0985", "0945", "or [ebp%+d],eax", 5 | A },
	{ xor_pbp_eax, "3185", "3145", "xor [ebp%+d],eax", 5 | A },
	{ sub_pbp_ecx, "298D", "294D", "sub [ebp%+d],ecx", 5 | C },
	{ sub_eax, "2D", "83E8", "sub eax,%d", 5 | A },
	{ sub_eax_pbp, "2B85", "2B45", "sub eax,[ebp%+d]", 5 | A },
	{ sub_eax_ecx, "2BC1", Z, "sub eax,ecx", AC },
	{ sub_pcx_eax, "2901", Z, "sub [ecx],eax", AC },
	{ sub_pcx_ax, "662901", Z, "sub [ecx],ax", AC },
	{ sub_pcx_al, "2801", Z, "sub [ecx],al", AC },
	{ sub_esp, "81EC", "83EC", "sub esp,%d", 5 },
	{ sub_dax, "8128", "8328", "sub dp[eax],%d", 5 | A },
	{ sub_dcx, "8129", "8329", "sub dp[ecx],%d", 5 | C },
	{ sub_ddx, "812A", "832A", "sub dp[edx],%d", 5 | D },
	{ sub_bax, Z, "8028", "sub bp[eax],%d", 1 | A },
	{ sub_bcx, Z, "8029", "sub bp[ecx],%d", 1 | C },
	{ sub_bdx, Z, "802A", "sub bp[edx],%d", 1 | D },
	{ imul_eax_ecx, "0FAFC1", Z, "imul eax,ecx", ACD }, 
	{ imul_eax_eax, "69C0", "6BC0", "imul eax,eax,%d", 5 | AD },
	{ imul_edx_edx, "69D2", "6BD2", "imul edx,edx,%d", 5 | AD },
	{ imul_eax_pbp, "0FAF85", "0FAF45", "imul eax,[ebp%+d]", 5 | A },
	{ xdiv_dbp, "99F7BD", "99F77D", "cdq&idiv dp[ebp%+d]", 5 | AD },
	{ cmp_eax_ecx, "39C8", Z, "cmp eax,ecx", AC },
	{ cmp_ecx_eax, "3BC8", Z, "cmp ecx,eax", AC },
	{ cmp_eax, "81F8", "83F8", "cmp eax,%d", 5 | A },
	{ cmp_eax_pbp, "3B85", "3B45", "cmp eax,[ebp%+d]", 5 | A },
	{ cmp_ah, Z, "80FC", "cmp ah,%d", 1 | A },
	{ test_ah, Z, "F6C4", "test ah,%d", 1 | A },
	{ test_eax_eax, "85C0", Z, "test eax,eax", A },
	{ and_pbp_eax, "2185", "2145", "and [ebp%+d],eax", 5 | A },
	{ and_eax_ecx, "23C1", Z, "and eax,ecx", AC },
	{ and_pcx_eax, "2101", Z, "and [ecx],eax", AC },
	{ and_pcx_ax, "662101", Z, "and [ecx],ax", AC },
	{ and_pcx_al, "2001", Z, "and [ecx],al", AC },
	{ and_ah, Z, "80E4", "and ah,%d", 1 | A },
	{ or_eax_ecx, "0BC1", Z, "or eax,ecx", AC },
	{ or_pcx_eax, "0901", Z, "or [ecx],eax", AC },
	{ or_pcx_ax, "660901", Z, "or [ecx],ax", AC },
	{ or_pcx_al, "0801", Z, "or [ecx],al", AC },
	{ xor_eax_eax, "31C0", Z, "xor eax,eax", A },
	{ xor_ah, Z, "80F4", "xor ah,%d", 1 | A },
	{ xor_pcx_ax, "663101", Z, "xor [ecx],ax", AC },
	{ xor_pcx_eax, "3101", Z, "xor [ecx],eax", AC },
	{ xor_eax_ecx, "33C1", Z, "xor eax,ecx", AC },
	{ xor_pcx_al, "3001", Z, "xor [ecx],al", AC },
	{ shl_eax, Z, "C1E0", "shl eax,%d", 1 | A },
	{ shl_edx, Z, "C1E2", "shl edx,%d", 1 | D },
	{ shl_eax_cl, "D3E0", Z, "shl eax,cl", AC },
	{ shr_eax_cl, "D3E8", Z, "shr eax,cl", AC },
	{ neg_eax, "F7D8", Z, "neg eax", A },
	{ mov_eax_edx, "8BC2", Z, "mov eax,edx", AD },
	{ mov_ecx_eax, "8BC8", Z, "mov ecx,eax", AC },
	{ mov_edx_eax, "8BD0", Z, "mov edx,eax", AD },
	{ mov_eax, "B8", Z, "mov eax,%d", 4 | A },
	{ mov_ecx, "B9", Z, "mov ecx,%d", 4 | C },
	{ mov_dax, "C700", Z, "mov dp[eax],%d", 4 | A },
	{ mov_wax, "66C700", Z, "mov wp[eax],%d", 2 | A },
	{ mov_bax, Z, "C600", "mov bp[eax],%d", 1 | A },
	{ mov_eax_pax, "8B00", Z, "mov eax,[eax]", A },
	{ mov_eax_pcx, "8B01", Z, "mov eax,[ecx]", AC },
	{ mov_pcx_eax, "8901", Z, "mov [ecx],eax", AC },
	{ mov_pcx_ax, "668901", Z, "mov [ecx],ax", AC },
	{ mov_pax_cx, "668908", Z, "mov [eax],cx", AC },
	{ mov_pcx_al, "8801", Z, "mov [ecx],al", AC },
	{ mov_pax_cl, "8808", Z, "mov [eax],cl", AC },
	{ mov_pax_ecx, "8908", Z, "mov [eax],ecx", AC },
	{ mov_ecx_pax, "8B08", Z, "mov ecx,[eax]", AC },
	{ mov_edx_pax, "8B10", Z, "mov edx,[eax]", AD },
	{ mov_eax_pbp, "8B85", "8B45", "mov eax,[ebp%+d]", 5 | A },
	{ mov_ecx_pbp, "8B8D", "8B4D", "mov ecx,[ebp%+d]", 5 | C },
	{ mov_edx_pbp, "8B95", "8B55", "mov edx,[ebp%+d]", 5 | D },
	{ mov_pbp_ecx, "898D", "894D", "mov [ebp%+d],ecx", 5 | C },
	{ mov_pbp_eax, "8985", "8945", "mov [ebp%+d],eax", 5 | A },
	{ mov_pbp_al, "8885", "8845", "mov [ebp%+d],al", 5 | A },
	{ mov_psp_eax, "898424", "894424", "mov [esp%+d],eax", 5 | A },
	{ mov_eax_psp, Z, "8B4424", "mov eax,[esp%+d]", 1 | X },
	{ movsx_eax_wax, "0FBF00", Z, "movsx eax,wp[eax]", A },
	{ movsx_eax_bax, "0FBE00", Z, "movsx eax,bp[eax]", A },
	{ movsx_ecx_wcx, "0FBF09", Z, "movsx ecx,wp[ecx]", C }, 
	{ movsx_ecx_bcx, "0FBE09", Z, "movsx ecx,bp[ecx]", C }, 
	{ lea_eax_pbp, "8D85", "8D45", "lea eax,[ebp%+d]", 5 | A },
	{ lea_ecx_pbp, "8D8D", "8D4D", "lea ecx,[ebp%+d]", 5 | C },
	{ lea_edx_pbp, "8D95", "8D55", "lea edx,[ebp%+d]", 5 | D },
	{ mov_eax_ad1, "8B0402", Z, "mov eax,[eax+edx]", AD },
	{ mov_eax_ad2, "8B0450", Z, "mov eax,[eax+edx*2]", AD },
	{ mov_eax_ad4, "8B0490", Z, "mov eax,[eax+edx*4]", AD },
	{ mov_eax_ad8, "8B04D0", Z, "mov eax,[eax+edx*8]", AD },
	{ mov_eax_da1, "8B0410", Z, "mov eax,[edx+eax]", AD },
	{ mov_eax_da2, "8B0442", Z, "mov eax,[edx+eax*2]", AD },
	{ mov_eax_da4, "8B0482", Z, "mov eax,[edx+eax*4]", AD },
	{ mov_eax_da8, "8B04C2", Z, "mov eax,[edx+eax*8]", AD },
	{ lea_eax_ad1, "8D0402", Z, "lea eax,[eax+edx]", AD },
	{ lea_eax_ad2, "8D0450", Z, "lea eax,[eax+edx*2]", AD },
	{ lea_eax_ad4, "8D0490", Z, "lea eax,[eax+edx*4]", AD },
	{ lea_eax_ad8, "8D04D0", Z, "lea eax,[eax+edx*8]", AD },
	{ lea_eax_da1, "8D0410", Z, "lea eax,[edx+eax]", AD },
	{ lea_eax_da2, "8D0442", Z, "lea eax,[edx+eax*2]", AD },
	{ lea_eax_da4, "8D0482", Z, "lea eax,[edx+eax*4]", AD },
	{ lea_eax_da8, "8D04C2", Z, "lea eax,[edx+eax*8]", AD },
	{ lea_ecx_da1, "8D0C10", Z, "lea ecx,[edx+eax]", ACD },
	{ lea_ecx_da2, "8D0C42", Z, "lea ecx,[edx+eax*2]", ACD },
	{ lea_ecx_da4, "8D0C82", Z, "lea ecx,[edx+eax*4]", ACD },
	{ lea_ecx_da8, "8D0CC2", Z, "lea ecx,[edx+eax*8]", ACD },
	{ xchg_eax_ecx, "91", Z, "xchg eax,ecx", AC },
	{ cwde, "98", Z, "cwde", A },
	{ jmp, "E9", "EB", "jmp ", 5 | X },
	{ jz, "0F84", "74", "jz ", 5 | X },
	{ jnz, "0F85", "75", "jnz ", 5 | X },
	{ jl, "0F8C", "7C", "jl ", 5 | X },
	{ jge, "0F8D", "7D", "jge ", 5 | X },
	{ jle, "0F8E", "7E", "jle ", 5 | X },
	{ jg, "0F8F", "7F", "jg ", 5 | X },
	{ call, "E8", Z, "call fn_%04d", 4 | X },
	{ xent, "5589E5", Z, "push ebp&mov ebp,esp" },
	{ xret, "C9C3", "C9C2", "leave&ret", X },
	{ sete_eax, "B8000000000F94C0", Z, "sete eax", X },
	{ setne_eax, "B8000000000F95C0", Z, "setne eax", X },
	{ setl_eax, "B8000000000F9CC0", Z, "setl eax", X },
	{ setge_eax, "B8000000000F9DC0", Z, "setge eax", X },
	{ setle_eax, "B8000000000F9EC0", Z, "setle eax", X },
	{ setg_eax, "B8000000000F9FC0", Z, "setg eax", X },
	{ fchs, "D9E0", Z, "fchs" },
	{ fxch_st1, "D9C9", Z, "fxch st(1)" },
	{ fld_qax, "DD00", Z, "fld qp[eax]", A },
	{ fld_qcx, "DD01", Z, "fld qp[ecx]", C },
	{ fst_qax, "DD10", Z, "fst qp[eax]", A },
	{ fst_qcx, "DD11", Z, "fst qp[ecx]", C },
	{ fld_qbp, "DD85", "DD45", "fld qp[ebp%+d]", 5 },
	{ fld_qp, "DD05", Z, "fld qp[off_%p]", 4 },
	{ fldcw, "D92D", Z, "fldcw [off_%p]", 4 },
	{ fstp_qsp, "DD5C2400", Z, "fstp qp[esp]" },
	{ fstsw, "DFE0", Z, "fstsw" },
	{ fstp_st1, "DDD9", Z, "fstp st1" },
	{ fst_qbp, "DD95", "DD55", "fst qp[ebp%+d]", 5 },
	{ fadd_qbp, "DC85", "DC45", "fadd qp[ebp%+d]", 5 },
	{ fsub_qbp, "DCA5", "DC65", "fsub qp[ebp%+d]", 5 },
	{ fmul_qbp, "DC8D", "DC4D", "fmul qp[ebp%+d]", 5 },
	{ fdiv_qbp, "DCB5", "DC75", "fdiv qp[ebp%+d]", 5 },
	{ fadd_qp, "DC05", Z, "fadd qp[%d]", 4 },
	{ fsub_qp, "DC25", Z, "fsub qp[%d]", 4 },
	{ fmul_qp, "DC0D", Z, "fmul qp[%d]", 4 },
	{ fdiv_qp, "DC35", Z, "fdiv qp[%d]", 4 },
	{ faddp_st1_st, "DEC1", Z, "faddp st1,st" },
	{ fmulp_st1_st, "DEC9", Z, "fmulp st1,st" },
	{ fsubrp_st1_st, "DEE9", Z, "fsubrp st1,st" },
	{ fdivrp_st1_st, "DEF9", Z, "fdivrp st1,st" },
	{ fistp_dsp, Z, "DB5C24", "fistp dp[esp%+d]", 1 },
	{ fild_dax, "DB00", Z, "fild dp[eax]", A },
	{ fild_dsp, "DB8424", "DB4424", "fild dp[esp%+d]", 5 },
	{ fucompp, "DAE9", Z, "fucompp" },
	{ xdiv_ecx, "99F7F9", Z, "cdq&idiv ecx", X },
	{ xmod_ecx, "99F7F989D0", Z, "cdq&idiv ecx&mov eax,edx", X },
	{ setint, "", Z, "setint %d @%d" },
	{ setreal, "", Z, "setreal %.3f @%d" },
	{ setstr, "", Z, "setstr '%s' @%d" },
	{ fn_, Z, Z, "fn_%04d:", 4 },
	{ exp_, Z, Z, "exp fn_%04d:", 4 },
	{ loc_, Z, Z, "loc_%03d:", 4 },
};

/*============================================================================
 * Inicialização de Instruções
 *============================================================================*/

static int cmpInst(const void *a, const void *b)
{
	return ((INSTRUCTION*)a)->opcode - ((INSTRUCTION*)b)->opcode;
}

void initInstruction(void)
{
	int n, size1 = sizeof(INSTRUCTION);
	qsort(instruction, sizeof(instruction) / size1, size1, cmpInst);
	for (n = 0; n < sizeof(instruction) / size1; n++)
	{
		INSTRUCTION *pI = &instruction[n];
		if (pI->opcode != n + 1)
			error("INST", "mismatch! %s opcode=%d n+1=%d", pI->mnemonic, pI->opcode, n + 1);
	}
}

/*============================================================================
 * Gerenciamento de DLLs (apenas Windows)
 *============================================================================*/

#ifdef PLATFORM_WINDOWS

void initDLL(char *dllnames)
{
	int i, n;
	char *dllname = strtok(dllnames, ";");
	for (n = 0; dllname != NULL; dllname = strtok(NULL, ";"))
	{
		ULONG hM = (ULONG)LoadLibrary(dllname);
		if (hM == 0) error("exe.initDLL", "DLL '%s'", dllname);
		IMAGE_DOS_HEADER *pDOSHeader = (IMAGE_DOS_HEADER *)hM;
		IMAGE_NT_HEADERS *pPEHeader = (IMAGE_NT_HEADERS *)(hM + pDOSHeader->e_lfanew);
		IMAGE_DATA_DIRECTORY *pD = pPEHeader->OptionalHeader.DataDirectory;
		IMAGE_EXPORT_DIRECTORY *pE = (IMAGE_EXPORT_DIRECTORY*)(hM + pD[0].VirtualAddress);
		char **ppszFunctionName = (char**)((int)pE->AddressOfNames + hM);
		for (i = 0; i < (int)pE->NumberOfNames; i++)
		{
			char *name = (char*)(ppszFunctionName[i] + hM);
			int ix = put(strncmp(name, "__p_", 4) == 0 ? (name + 4) : name, NULL, &cd.hash);
			cd.hash.tbl[ix].val = (void*)(AT_IMPT | n);
		}
		FreeLibrary((HMODULE)hM);
		exe.dll[n++].dllname = dllname;
	}
	exe.nDLL = n;
}

void setDLL(int ixDLL, int id)
{
	int n = 0;
	while (n < exe.dll[ixDLL].nFunc && exe.dll[ixDLL].idFunc[n] != id)
		n++;
	if (n >= exe.dll[ixDLL].nFunc)
	{
		exe.dll[ixDLL].nFunc++;
		exe.dll[ixDLL].idFunc[n] = id;
	}
}

int importDLL(void)
{
	int k, n, size = 0;
	for (n = 0; n < exe.nDLL; n++)
	{
		if (exe.dll[n].nFunc == 0) continue;
		size += strlen(exe.dll[n].dllname) + 1;
		exe.useDLL++;
		for (k = 0; k < exe.dll[n].nFunc; k++)
			size += strlen(toString(exe.dll[n].idFunc[k])) + 3;
		exe.useFunc += exe.dll[n].nFunc;
	}
	size += exe.useDLL * 20 + 20;
	size += (exe.useFunc + exe.useDLL) * 4 * 2;
	size = ((size - 1) / 16 + 1) * 16;
	return size;
}

BYTE *getImport(void)
{
	int i, n = 0, k;
	DWORD *pImpt = calloc(exe.lenImpt, 1);
	if (pImpt == NULL) error("exe.getImport", "Error to import function");
	int nLookup = (exe.useDLL + 1) * 5;
	int nImptAddr = nLookup + (exe.useDLL + exe.useFunc);
	int nName = (nImptAddr + (exe.useDLL + exe.useFunc)) * 4;
	for (i = 0; i < exe.nDLL; i++)
	{
		if (exe.dll[i].nFunc == 0) continue;
		pImpt[n * 5] = mem.ImptAddr + nLookup * 4;
		pImpt[n * 5 + 4] = mem.ImptAddr + nImptAddr * 4;
		for (k = 0; k < exe.dll[i].nFunc; k++)
		{
			int ix = exe.dll[i].idFunc[k];
			char *fname = toString(ix);
			cd.hash.tbl[ix].val = (void*)(AT_IMPT | (pImpt[n * 5 + 4] + k * 4));
			pImpt[nLookup++] = mem.ImptAddr + nName;
			pImpt[nImptAddr++] = mem.ImptAddr + nName;
			strcpy((char*)pImpt + nName + 2, fname);
			nName += strlen(fname) + 3;
		}
		nLookup++;
		nImptAddr++;
		pImpt[n * 5 + 3] = mem.ImptAddr + nName;
		strcpy((char*)pImpt + nName, exe.dll[i].dllname);
		nName += strlen(exe.dll[i].dllname) + 1;
		n++;
	}
	return (BYTE*)pImpt;
}

/*============================================================================
 * Export
 *============================================================================*/

typedef struct _Export
{
	int  addr;
	char *name;
} Export;

static Export export[100];
static int nExport;

static int cmpName(const void *a, const void *b)
{
	return strcmp(((Export *)a)->name, ((Export *)b)->name);
}

int initExport(void)
{
	int n;
	int size = strlen(cmd.outfile) + 1;
	for (n = 0; n < cd.hash.size; n++)
	{
		char *name = cd.hash.tbl[n].key;
		int val = (int)cd.hash.tbl[n].val;
		if (name != NULL && (val&AT_EXPT) != 0)
		{
			export[nExport].addr = mem.CodeAddr + (val&AT_ADDR);
			export[nExport++].name = name;
			size += 10 + strlen(name) + 1;
		}
	}
	qsort(export, nExport, sizeof(Export), cmpName);
	return sizeof(IMAGE_EXPORT_DIRECTORY) + size;
}

BYTE *getExport(void)
{
	int n;
	int sizeIED = sizeof(IMAGE_EXPORT_DIRECTORY);
	int baseMem = mem.ExptAddr;
	int baseRVA = baseMem + sizeIED;
	char *buf = xalloc(exe.lenExpt);
	IMAGE_EXPORT_DIRECTORY *pIed = (IMAGE_EXPORT_DIRECTORY *)buf;
	IMAGE_EXPORT_DIRECTORY ied =
	{
		0, 0, 0, 0, baseRVA + 10 * nExport,
		1, nExport, nExport, baseRVA, baseRVA + 4 * nExport, baseRVA + 8 * nExport
	};
	memcpy(buf, &ied, sizeof(ied));
	DWORD *pAddr = (DWORD*)(buf + sizeIED);
	DWORD *pName = (DWORD*)(buf + sizeIED + 4 * nExport);
	WORD  *pOrd = (WORD*)(buf + sizeIED + 8 * nExport);
	char  *pStr = buf + sizeIED + 10 * nExport;
	for (n = 0; n < nExport; n++)
		pAddr[n] = export[n].addr;
	strcpy(pStr, cmd.outfile);
	pStr += strlen(cmd.outfile) + 1;
	for (n = 0; n < nExport; n++)
	{
		pName[n] = baseMem + (pStr - buf);
		strcpy(pStr, export[n].name);
		pStr += strlen(export[n].name) + 1;
		pOrd[n] = n;
	}
	return (BYTE*)buf;
}

/*============================================================================
 * Relocation
 *============================================================================*/

static int cmpAddr(const void *a, const void *b)
{
	return *((int *)a) - *((int *)b);
}

void initReloc(void)
{
	int n;
	qsort(exe.locs, exe.nLocs, sizeof(int), cmpAddr);
	exe.nPages = exe.nLocs > 0 ? (exe.locs[exe.nLocs - 1] - 1) / 4096 : 0;
	for (n = 0; n < exe.nLocs; n++)
		exe.cnt[exe.locs[n] / 4096 - 1]++;
	mem.RelocAddr = mem.DataAddr + (mem.DataSize + MEMALIGN1) & ~MEMALIGN1;
	mem.RelocSize = exe.nPages * 8 + exe.nLocs * 2;
	raw.RelocAddr = raw.DataAddr + raw.DataSize;
	raw.RelocSize = (mem.RelocSize + RAWALIGN1) & ~RAWALIGN1;
}

BYTE *getReloc(void)
{
	int n, k, ix = 0;
	BYTE *buf = xalloc(raw.RelocSize);
	WORD *p = (WORD*)buf;
	for (n = 0; n < exe.nPages; n++)
	{
		int *q = (int*)p;
		*q++ = (n + 1) * 4096;
		*q++ = 8 + exe.cnt[n] * 2;
		p = (WORD*)q;
		for (k = 0; k < exe.cnt[n]; k++)
			*p++ = 0x3000 + (exe.locs[ix++] & 0x0FFF);
	}
	return buf;
}

/*============================================================================
 * Escrita do Executável PE
 *============================================================================*/

typedef struct _EXE_HEADER
{
	IMAGE_DOS_HEADER      DosHeader;
	BYTE 		  DosStub[64];
	DWORD                 Signature;
	IMAGE_FILE_HEADER     FileHeader;
	IMAGE_OPTIONAL_HEADER OptionalHeader;
	IMAGE_SECTION_HEADER  SectionHeaders[2];
} EXE_HEADER;

void writeExe(BYTE *bufImport, BYTE *bufExport, BYTE *CodeBuffer,
	int lenCode, BYTE *DataBuffer, int lenData)
{
	BYTE *bufReloc = NULL;
	int sizeImptAddrTable = (exe.useDLL + exe.useFunc) * 4;
	int posImptAddrTable = mem.DataAddr + (exe.useDLL + 1) * 20 + sizeImptAddrTable;
	int numSections = mcc.typeApp == 0 ? 3 : 2;
	int typeApp = mcc.typeApp == 0 ? 2 : mcc.typeApp;
	int exptAddr = mcc.typeApp == 0 ? mem.ExptAddr : 0;
	EXE_HEADER exeHeader =
	{
		{
			0x5A4D, 0x90, 3, 0, 4,
			0, 0xFFFF, 0, 0xB8,
			0, 0, 0, 0x40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0x0080,
		},
		"\x0E\x1F\xBA\x0E\x00\xB4\x09\xCD\x21\xB8\x01\x4C\xCD\x21"
		"This program cannot be run in DOS mode.\r\r\n$",
		0x00004550,
		{
			0x014c, numSections, 0, 0, 0,
			sizeof(IMAGE_OPTIONAL_HEADER), 0x030F
		}, {
			0x010B, 0x06, 0x00,
			0, 0, 0, exe.entryPoint,
			mem.CodeAddr, mem.DataAddr,
			exe.base, MEMALIGN, RAWALIGN,
			4, 0, 0, 0, 4, 0, 0,
			exe.sizeImage, RAWALIGN, 0,
			typeApp, 0,
			0x100000, 0x1000, 0x100000, 0x1000,
			0, 0x10,
			{ { exptAddr, exe.lenExpt }, { mem.ImptAddr, (exe.useDLL + 1) * 20 },
			{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
			{ posImptAddrTable, sizeImptAddrTable }, { 0, 0 }, { 0, 0 }, { 0, 0 }
			}
		}, { {
				".text", { mem.CodeSize }, mem.CodeAddr, raw.CodeSize,
				raw.CodeAddr, 0, 0, 0, 0, 0x60000020
			}, {
				".data", { mem.DataSize }, mem.DataAddr, raw.DataSize,
				raw.DataAddr, 0, 0, 0, 0, 0xC0000040
			}
		}
	};
	IMAGE_SECTION_HEADER reloc =
	{
		".reloc", { mem.RelocSize }, mem.RelocAddr, raw.RelocSize, raw.RelocAddr,
		0, 0, 0, 0, 0x42000040
	};

	BYTE *bufEXE = xalloc(raw.DataAddr + raw.DataSize);
	memcpy(bufEXE, &exeHeader, sizeof(exeHeader));
	if (mcc.typeApp == 0)
	{
		memcpy(bufEXE + sizeof(exeHeader), &reloc, sizeof(reloc));
		bufReloc = getReloc();
	}
	memcpy(bufEXE + RAWALIGN, CodeBuffer, lenCode);
	memcpy(bufEXE + raw.ImptAddr, bufImport, exe.lenImpt);
	if (bufExport != NULL)
		memcpy(bufEXE + raw.ImptAddr + exe.lenImpt, bufExport, exe.lenExpt);
	memcpy(bufEXE + raw.ImptAddr + exe.lenImpt + exe.lenExpt, DataBuffer, lenData);
	FILE *fpDst = fopen(cmd.outfile, "wb");
	if (fpDst == NULL) error("writeExe", "'%s' open error", cmd.outfile);
	fwrite(bufEXE, raw.DataAddr + raw.DataSize, 1, fpDst);
	if (bufReloc != NULL) fwrite(bufReloc, raw.RelocSize, 1, fpDst);
	fclose(fpDst);
}

/*============================================================================
 * Linker
 *============================================================================*/

static void instAt(int n, int *pinst, int *pnum, int *pattr, int *psize)
{
	*pinst = cd.pCode[n].inst;
	*pnum = cd.pCode[n].num;
	*pattr = cd.pCode[n].attr;
	*psize = cd.pCode[n].size;
}

static int setOffset(int *pLoc, int *pOffCode, int cnt)
{
	int n, inst, num, attr, size, nChg = 0;
	int offCode = 0;
	for (n = 0; n < ix.ixCode; n++)
	{
		instAt(n, &inst, &num, &attr, &size);
		INSTRUCTION *pI = &instruction[inst - 1];
		char *fmt = pI->mnemonic;
		if ((opt&oASM) && cnt == 0)
		{
			printf((strchr(fmt, ':') != NULL ? "%4d: " : "%4d:     "), n);
			if (inst == setreal)
				printf(fmt, cd.pCode[n].dval, cd.pCode[n].offset);
			else if (inst == setstr)
				printf(fmt, cd.pCode[n].sval, cd.pCode[n].offset);
			else
				printf(fmt, num, cd.pCode[n].offset);
			if (fmt[0] == 'j') printf(attr == AD_CONST ? "%d" : "loc_%03d", num);
			else if (inst == xret && num > 0) printf(" %d", num);
			else if (attr != 0)	printf("[%d]", attr);
			if (pI->opcode == fn_ || pI->opcode == call)
				printf("[%s]", cd.hash.tbl[num].key);
			printf("\n");
		}
		if (setint <= inst && inst <= setstr) continue;
		if (cnt == 0) cd.pCode[n].size = 0;
		cd.pCode[n].offset = offCode;
		if (pI->opcode == loc_ && num <= ix.ixLoc)
		{
			pLoc[num] = offCode;
		}
		else if (pI->opcode == fn_ || pI->opcode == exp_)
		{
			if (strcmp(cd.hash.tbl[num].key, "_main") == 0)
				exe.entryPoint = offCode;
			int at = ((int)cd.hash.tbl[num].val) & 0xFF000000;
			cd.hash.tbl[num].val = (void*)(at + offCode);
		}
		else if (pI->mnemonic[0] == 'j')
		{
			int fByte = abs(pLoc[num] - offCode - 2) < 128;
			int fS = (opt&oUOPT) ? (pLoc[num] > 0 && fByte) : (cnt == 0 || fByte);
			fS = (fS || attr == AD_CONST);
			offCode += strlen(fS ? pI->hexcode2 : pI->hexcode) / 2 + (fS ? 1 : 4);
			int sizeOld = cd.pCode[n].size;
			cd.pCode[n].size = fS ? 1 : 4;
			if (cd.pCode[n].size != sizeOld) nChg++;
		}
		else if (pI->opcode == call)
		{
			int fImpt = ((int)cd.hash.tbl[num].val) & AT_IMPT;
			offCode += fImpt ? 6 : 5;
			if (fImpt && cnt == 0) setDLL((int)cd.hash.tbl[num].val & AT_ADDR, num);
		}
		else if (pI->opcode == xret)
		{
			offCode += num == 0 ? 2 : 4;
			cd.pCode[n].size = num == 0 ? 0 : 2;
		}
		else if (abs(num) < 128 && (pI->regs_size & 0x01) == 1)
		{
			offCode += strlen(pI->hexcode2) / 2 + 1;
			cd.pCode[n].size = 1;
		}
		else if (abs(num) < (1 << 15) && (pI->regs_size & 0x02) == 2)
		{
			offCode += strlen(pI->hexcode2) / 2 + 2;
			cd.pCode[n].size = 2;
		}
		else if (pI->hexcode != NULL && pI->hexcode[0] != '\0')
		{
			offCode += strlen(pI->hexcode) / 2 + (pI->regs_size & 0x04);
			cd.pCode[n].size = pI->regs_size & 0x04;
		}
		else
		{
			error("linker", "mnemonic=%s, num=%d", pI->mnemonic, num);
		}
		if (cnt == 0 && attr == AD_IMPORT)
			setDLL((int)cd.hash.tbl[num].val & AT_ADDR, num);
	}
	*pOffCode = offCode;
	return nChg;
}

void link(void)
{
	int  n, inst, num, attr, size, offset, offCode, cnt = 0, nChg;
	char *p, *hexcode, *caText;

	int *pLoc = xalloc((ix.ixLoc + 1) * sizeof(int));
	initDLL(cmd.impfiles);
	do
	{
		nChg = setOffset(pLoc, &offCode, cnt++);
	} while (!(opt&oUOPT) && nChg > 0);

	mem.CodeAddr = MEMALIGN;
	mem.CodeSize = offCode;
	raw.CodeAddr = RAWALIGN;
	raw.CodeSize = (offCode + RAWALIGN1) & ~RAWALIGN1;

	exe.lenImpt = importDLL();
	exe.lenExpt = (opt&oDLL) ? initExport() : 0;
	int posData = exe.lenImpt + exe.lenExpt;


	mem.DataAddr = mem.CodeAddr + (offCode + MEMALIGN1) & ~MEMALIGN1;
	mem.DataSize = exe.lenImpt + exe.lenExpt + ix.ixData + ix.ixZero;

	raw.DataAddr = raw.CodeAddr + raw.CodeSize;
	raw.DataSize = (exe.lenImpt + exe.lenExpt + ix.ixData + RAWALIGN1) & ~RAWALIGN1;
	mem.ImptAddr = mem.DataAddr;
	raw.ImptAddr = raw.DataAddr;
	mem.ExptAddr = mem.ImptAddr + exe.lenImpt;
	raw.ExptAddr = raw.ImptAddr + exe.lenImpt;

	exe.base = (opt&oDLL) ? DLLBASE : IMAGEBASE;
	exe.entryPoint += mem.CodeAddr;
	BYTE *bufImport = getImport();
	BYTE *bufExport = (opt&oDLL) ? getExport() : NULL;
	p = caText = xalloc(raw.CodeSize);
	int dataAddr = exe.base + mem.DataAddr + posData;
	for (n = 0; n < ix.ixCode; n++)
	{
		BYTE  *q;
		instAt(n, &inst, &num, &attr, &size);
		offset = cd.pCode[n].offset;
		INSTRUCTION *pI = &instruction[inst - 1];
		hexcode = size == 1 ? pI->hexcode2 : pI->hexcode;
		if (hexcode == NULL) continue;
		int fReloc = TRUE;
		if (pI->opcode == call)
		{
			int fImpt = (int)cd.hash.tbl[num].val & AT_IMPT;
			int fUser = (int)cd.hash.tbl[num].val & AT_USER;
			int addr = (int)cd.hash.tbl[num].val & AT_ADDR;
			if (!fImpt && !fUser) error("linker", "'%s' undeclared", toString(num));
			num = fImpt ? (exe.base + addr) : (addr - cd.pCode[n + 1].offset);
			hexcode = fImpt ? "FF15" : "E8";
			if (!fImpt)	fReloc = FALSE;
			size = 4;
		}
		else if (pI->mnemonic[0] == 'j')
		{
			int sizeInst = size == 1 ? 2 : inst == jmp ? 5 : 6;
			if (attr != AD_CONST) num = pLoc[num] - (cd.pCode[n].offset + sizeInst);
			fReloc = FALSE;
		}
		else if (attr == AD_CODE)
		{
			int foffset = (int)cd.hash.tbl[num].val & AT_ADDR;
			num = exe.base + mem.CodeAddr + foffset;
		}
		else if (attr == AD_DATA)
		{
			num += dataAddr;
		}
		else if (attr == AD_ZERO)
		{
			num += dataAddr + ix.ixData;
		}
		else if (attr == AD_IMPORT)
		{
			num = exe.base + ((int)cd.hash.tbl[num].val&AT_ADDR);
		}
		else
		{
			fReloc = FALSE;
		}
		for (q = (BYTE*)hexcode; *q != '\0'; q += 2)
			*p++ = (htoi(*q) << 4) + htoi(q[1]);
		if (size == 1) *p = num;
		else if (size == 2) *((short*)p) = num;
		else if (size == 4) *((int*)p) = num;
		if (fReloc) exe.locs[exe.nLocs++] = num - exe.base;
		p += size;
	}
	char  *caData = calloc(ix.ixData + 1, sizeof(char));
	for (n = 0; n < ix.ixCode; n++)
	{
		instAt(n, &inst, &num, &attr, &size);
		offset = cd.pCode[n].offset;
		if (inst < setint || setstr < inst) continue;
		if (inst == setint && attr == 1) caData[offset] = num;
		else if (inst == setint && attr == 2) *((short*)&caData[offset]) = num;
		else if (inst == setint && attr == 4) *((int*)&caData[offset]) = num;
		else if (inst == setint) *((int*)&caData[offset]) = num + dataAddr;
		else if (inst == setreal) *((double*)&caData[offset]) = cd.pCode[n].dval;
		else xstrcpy(&caData[offset], cd.pCode[n].sval);
	}
	if (opt&oDLL)
	{
		initReloc();
		exe.sizeImage = mem.RelocAddr + (mem.RelocSize + MEMALIGN1) & ~MEMALIGN1;
	}
	else
	{
		exe.sizeImage = mem.DataAddr + (mem.DataSize + MEMALIGN1) & ~MEMALIGN1;
	}
	writeExe(bufImport, bufExport, (BYTE*)caText, offCode, (BYTE*)caData, ix.ixData);
}

#endif /* PLATFORM_WINDOWS */
