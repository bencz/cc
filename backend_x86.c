/* x86 instruction encoding and direct PE32 image generation. */

#include "cc.h"
#include "pe_format.h"

static void writeU16Le(unsigned char *destination, unsigned int value)
{
	destination[0] = (unsigned char)value;
	destination[1] = (unsigned char)(value >> 8);
}

static void writeU32Le(unsigned char *destination, unsigned int value)
{
	destination[0] = (unsigned char)value;
	destination[1] = (unsigned char)(value >> 8);
	destination[2] = (unsigned char)(value >> 16);
	destination[3] = (unsigned char)(value >> 24);
}

static void writeDoubleLe(unsigned char *destination, double value)
{
	unsigned char bytes[sizeof(double)];
	unsigned int endianProbe = 1U;
	int index;
	if (sizeof(double) != 8U)
	{
		error("PE writer", "the host double format is not 64-bit IEEE-754");
	}
	memcpy(bytes, &value, sizeof(value));
	if (*(unsigned char *)&endianProbe != 0U)
	{
		memcpy(destination, bytes, sizeof(bytes));
		return;
	}
	for (index = 0; index < 8; ++index)
	{
		destination[index] = bytes[7 - index];
	}
}

/* The parser uses instruction metadata even when another backend is selected. */

/* x86 instruction table */

#define Z ((void *)0)

INSTRUCTION x86Instructions[] = {
    {push, "68", Z, "push %d", 4},
    {push_pbp, "FFB5", "FF75", "push [ebp%+d]", 5},
    {push_eax, "50", Z, "push eax", A},
    {push_ecx, "51", Z, "push ecx", C},
    {pop_eax, "58", Z, "pop eax", A},
    {pop_ecx, "59", Z, "pop ecx", C},
    {pop_edx, "5A", Z, "pop edx", D},
    {inc_dbp, "FF85", "FF45", "inc dp[ebp%+d]", 5},
    {dec_dbp, "FF8D", "FF4D", "dec dp[ebp%+d]", 5},
    {add_eax_ecx, "03C1", Z, "add eax,ecx", AC},
    {add_eax_edx, "03C2", Z, "add eax,edx", AD},
    {add_eax, "05", "83C0", "add eax,%d", 5 | A},
    {add_ecx, "81C1", "83C1", "add ecx,%d", 5 | C},
    {add_pcx_eax, "0101", Z, "add [ecx],eax", AC},
    {add_pcx_ax, "660101", Z, "add [ecx],ax", AC},
    {add_pcx_al, "0001", Z, "add [ecx],al", AC},
    {add_esp, "81C4", "83C4", "add esp,%d", 5},
    {add_bax, "8000", Z, "add bp[eax]", A},
    {add_bcx, "8001", Z, "add bp[ecx]", C},
    {add_bdx, "8002", Z, "add bp[edx]", D},
    {add_dax, "8100", "8300", "add dp[eax],%d", 5 | A},
    {add_dcx, "8101", "8301", "add dp[ecx],%d", 5 | C},
    {add_ddx, "8102", "8302", "add dp[edx],%d", 5 | D},
    {add_wax, Z, "668300", "add wp[eax],%d", 1 | A},
    {add_wdx, Z, "668302", "add wp[edx],%d", 1 | D},
    {add_eax_pbp, "0385", "0345", "add eax,[ebp%+d]", 5 | A},
    {add_ecx_pbp, "038D", "034D", "add ecx,[ebp%+d]", 5 | C},
    {add_pbp_eax, "0185", "0145", "add [ebp%+d],eax", 5 | A},
    {add_pbp_ecx, "018D", "014D", "add [ebp%+d],ecx", 5 | C},
    {sub_pbp_eax, "2985", "2945", "sub [ebp%+d],eax", 5 | A},
    {or_pbp_eax, "0985", "0945", "or [ebp%+d],eax", 5 | A},
    {xor_pbp_eax, "3185", "3145", "xor [ebp%+d],eax", 5 | A},
    {sub_pbp_ecx, "298D", "294D", "sub [ebp%+d],ecx", 5 | C},
    {sub_eax, "2D", "83E8", "sub eax,%d", 5 | A},
    {sub_eax_pbp, "2B85", "2B45", "sub eax,[ebp%+d]", 5 | A},
    {sub_eax_ecx, "2BC1", Z, "sub eax,ecx", AC},
    {sub_pcx_eax, "2901", Z, "sub [ecx],eax", AC},
    {sub_pcx_ax, "662901", Z, "sub [ecx],ax", AC},
    {sub_pcx_al, "2801", Z, "sub [ecx],al", AC},
    {sub_esp, "81EC", "83EC", "sub esp,%d", 5},
    {sub_dax, "8128", "8328", "sub dp[eax],%d", 5 | A},
    {sub_dcx, "8129", "8329", "sub dp[ecx],%d", 5 | C},
    {sub_ddx, "812A", "832A", "sub dp[edx],%d", 5 | D},
    {sub_wax, Z, "668328", "sub wp[eax],%d", 1 | A},
    {sub_wdx, Z, "66832A", "sub wp[edx],%d", 1 | D},
    {sub_bax, Z, "8028", "sub bp[eax],%d", 1 | A},
    {sub_bcx, Z, "8029", "sub bp[ecx],%d", 1 | C},
    {sub_bdx, Z, "802A", "sub bp[edx],%d", 1 | D},
    {imul_eax_ecx, "0FAFC1", Z, "imul eax,ecx", ACD},
    {imul_eax_eax, "69C0", "6BC0", "imul eax,eax,%d", 5 | AD},
    {imul_edx_edx, "69D2", "6BD2", "imul edx,edx,%d", 5 | AD},
    {imul_eax_pbp, "0FAF85", "0FAF45", "imul eax,[ebp%+d]", 5 | A},
    {xdiv_dbp, "99F7BD", "99F77D", "cdq&idiv dp[ebp%+d]", 5 | AD},
    {cmp_eax_ecx, "39C8", Z, "cmp eax,ecx", AC},
    {cmp_ecx_eax, "3BC8", Z, "cmp ecx,eax", AC},
    {cmp_eax, "81F8", "83F8", "cmp eax,%d", 5 | A},
    {cmp_eax_pbp, "3B85", "3B45", "cmp eax,[ebp%+d]", 5 | A},
    {ucmp_eax_ecx, "39C8", Z, "cmp eax,ecx", AC},
    {ucmp_ecx_eax, "3BC8", Z, "cmp ecx,eax", AC},
    {ucmp_eax, "81F8", "83F8", "cmp eax,%d", 5 | A},
    {ucmp_eax_pbp, "3B85", "3B45", "cmp eax,[ebp%+d]", 5 | A},
    {cmp_ah, Z, "80FC", "cmp ah,%d", 1 | A},
    {test_ah, Z, "F6C4", "test ah,%d", 1 | A},
    {test_eax_eax, "85C0", Z, "test eax,eax", A},
    {and_pbp_eax, "2185", "2145", "and [ebp%+d],eax", 5 | A},
    {and_eax_ecx, "23C1", Z, "and eax,ecx", AC},
    {and_pcx_eax, "2101", Z, "and [ecx],eax", AC},
    {and_pcx_ax, "662101", Z, "and [ecx],ax", AC},
    {and_pcx_al, "2001", Z, "and [ecx],al", AC},
    {and_ah, Z, "80E4", "and ah,%d", 1 | A},
    {or_eax_ecx, "0BC1", Z, "or eax,ecx", AC},
    {or_pcx_eax, "0901", Z, "or [ecx],eax", AC},
    {or_pcx_ax, "660901", Z, "or [ecx],ax", AC},
    {or_pcx_al, "0801", Z, "or [ecx],al", AC},
    {xor_eax_eax, "31C0", Z, "xor eax,eax", A},
    {xor_ah, Z, "80F4", "xor ah,%d", 1 | A},
    {xor_pcx_ax, "663101", Z, "xor [ecx],ax", AC},
    {xor_pcx_eax, "3101", Z, "xor [ecx],eax", AC},
    {xor_eax_ecx, "33C1", Z, "xor eax,ecx", AC},
    {xor_pcx_al, "3001", Z, "xor [ecx],al", AC},
    {shl_eax, Z, "C1E0", "shl eax,%d", 1 | A},
    {shl_edx, Z, "C1E2", "shl edx,%d", 1 | D},
    {sar_eax, Z, "C1F8", "sar eax,%d", 1 | A},
    {shr_eax, Z, "C1E8", "shr eax,%d", 1 | A},
    {shl_eax_cl, "D3E0", Z, "shl eax,cl", AC},
    {sar_eax_cl, "D3F8", Z, "sar eax,cl", AC},
    {shr_eax_cl, "D3E8", Z, "shr eax,cl", AC},
    {neg_eax, "F7D8", Z, "neg eax", A},
    {not_eax, "F7D0", Z, "not eax", A},
    {mov_eax_edx, "8BC2", Z, "mov eax,edx", AD},
    {mov_ecx_eax, "8BC8", Z, "mov ecx,eax", AC},
    {mov_edx_eax, "8BD0", Z, "mov edx,eax", AD},
    {mov_eax, "B8", Z, "mov eax,%d", 4 | A},
    {mov_ecx, "B9", Z, "mov ecx,%d", 4 | C},
    {mov_dax, "C700", Z, "mov dp[eax],%d", 4 | A},
    {mov_wax, "66C700", Z, "mov wp[eax],%d", 2 | A},
    {mov_bax, Z, "C600", "mov bp[eax],%d", 1 | A},
    {mov_eax_pax, "8B00", Z, "mov eax,[eax]", A},
    {mov_eax_pcx, "8B01", Z, "mov eax,[ecx]", AC},
    {mov_pcx_eax, "8901", Z, "mov [ecx],eax", AC},
    {mov_pcx_ax, "668901", Z, "mov [ecx],ax", AC},
    {mov_pax_cx, "668908", Z, "mov [eax],cx", AC},
    {mov_pcx_al, "8801", Z, "mov [ecx],al", AC},
    {mov_pax_cl, "8808", Z, "mov [eax],cl", AC},
    {mov_pax_ecx, "8908", Z, "mov [eax],ecx", AC},
    {mov_ecx_pax, "8B08", Z, "mov ecx,[eax]", AC},
    {mov_edx_pax, "8B10", Z, "mov edx,[eax]", AD},
    {mov_eax_pbp, "8B85", "8B45", "mov eax,[ebp%+d]", 5 | A},
    {mov_ecx_pbp, "8B8D", "8B4D", "mov ecx,[ebp%+d]", 5 | C},
    {mov_edx_pbp, "8B95", "8B55", "mov edx,[ebp%+d]", 5 | D},
    {mov_pbp_ecx, "898D", "894D", "mov [ebp%+d],ecx", 5 | C},
    {mov_pbp_eax, "8985", "8945", "mov [ebp%+d],eax", 5 | A},
    {mov_pbp_al, "8885", "8845", "mov [ebp%+d],al", 5 | A},
    {mov_psp_eax, "898424", "894424", "mov [esp%+d],eax", 5 | A},
    {mov_eax_psp, Z, "8B4424", "mov eax,[esp%+d]", 1 | X},
    {movsx_eax_wax, "0FBF00", Z, "movsx eax,wp[eax]", A},
    {movsx_eax_bax, "0FBE00", Z, "movsx eax,bp[eax]", A},
    {movzx_eax_wax, "0FB700", Z, "movzx eax,wp[eax]", A},
    {movzx_eax_bax, "0FB600", Z, "movzx eax,bp[eax]", A},
    {movsx_ecx_wcx, "0FBF09", Z, "movsx ecx,wp[ecx]", C},
    {movsx_ecx_bcx, "0FBE09", Z, "movsx ecx,bp[ecx]", C},
    {lea_eax_pbp, "8D85", "8D45", "lea eax,[ebp%+d]", 5 | A},
    {lea_ecx_pbp, "8D8D", "8D4D", "lea ecx,[ebp%+d]", 5 | C},
    {lea_edx_pbp, "8D95", "8D55", "lea edx,[ebp%+d]", 5 | D},
    {mov_eax_ad1, "8B0402", Z, "mov eax,[eax+edx]", AD},
    {mov_eax_ad2, "8B0450", Z, "mov eax,[eax+edx*2]", AD},
    {mov_eax_ad4, "8B0490", Z, "mov eax,[eax+edx*4]", AD},
    {mov_eax_ad8, "8B04D0", Z, "mov eax,[eax+edx*8]", AD},
    {mov_eax_da1, "8B0410", Z, "mov eax,[edx+eax]", AD},
    {mov_eax_da2, "8B0442", Z, "mov eax,[edx+eax*2]", AD},
    {mov_eax_da4, "8B0482", Z, "mov eax,[edx+eax*4]", AD},
    {mov_eax_da8, "8B04C2", Z, "mov eax,[edx+eax*8]", AD},
    {lea_eax_ad1, "8D0402", Z, "lea eax,[eax+edx]", AD},
    {lea_eax_ad2, "8D0450", Z, "lea eax,[eax+edx*2]", AD},
    {lea_eax_ad4, "8D0490", Z, "lea eax,[eax+edx*4]", AD},
    {lea_eax_ad8, "8D04D0", Z, "lea eax,[eax+edx*8]", AD},
    {lea_eax_da1, "8D0410", Z, "lea eax,[edx+eax]", AD},
    {lea_eax_da2, "8D0442", Z, "lea eax,[edx+eax*2]", AD},
    {lea_eax_da4, "8D0482", Z, "lea eax,[edx+eax*4]", AD},
    {lea_eax_da8, "8D04C2", Z, "lea eax,[edx+eax*8]", AD},
    {lea_ecx_da1, "8D0C10", Z, "lea ecx,[edx+eax]", ACD},
    {lea_ecx_da2, "8D0C42", Z, "lea ecx,[edx+eax*2]", ACD},
    {lea_ecx_da4, "8D0C82", Z, "lea ecx,[edx+eax*4]", ACD},
    {lea_ecx_da8, "8D0CC2", Z, "lea ecx,[edx+eax*8]", ACD},
    {xchg_eax_ecx, "91", Z, "xchg eax,ecx", AC},
    {cwde, "98", Z, "cwde", A},
    {jmp, "E9", "EB", "jmp ", 5 | X},
    {jz, "0F84", "74", "jz ", 5 | X},
    {jnz, "0F85", "75", "jnz ", 5 | X},
    {jl, "0F8C", "7C", "jl ", 5 | X},
    {jge, "0F8D", "7D", "jge ", 5 | X},
    {jle, "0F8E", "7E", "jle ", 5 | X},
    {jg, "0F8F", "7F", "jg ", 5 | X},
    {call, "E8", Z, "call fn_%04d", 4 | X},
    {call_eax, "FFD0", Z, "call eax", A},
    {xent, "5589E5", Z, "push ebp&mov ebp,esp", 0},
    {xret, "C9C3", "C9C2", "leave&ret", X},
    {sete_eax, "B8000000000F94C0", Z, "sete eax", X},
    {setne_eax, "B8000000000F95C0", Z, "setne eax", X},
    {setl_eax, "B8000000000F9CC0", Z, "setl eax", X},
    {setge_eax, "B8000000000F9DC0", Z, "setge eax", X},
    {setle_eax, "B8000000000F9EC0", Z, "setle eax", X},
    {setg_eax, "B8000000000F9FC0", Z, "setg eax", X},
    {setb_eax, "B8000000000F92C0", Z, "setb eax", X},
    {setae_eax, "B8000000000F93C0", Z, "setae eax", X},
    {setbe_eax, "B8000000000F96C0", Z, "setbe eax", X},
    {seta_eax, "B8000000000F97C0", Z, "seta eax", X},
    {fchs, "D9E0", Z, "fchs", 0},
    {fxch_st1, "D9C9", Z, "fxch st(1)", 0},
    {fld_qax, "DD00", Z, "fld qp[eax]", A},
    {fld_qcx, "DD01", Z, "fld qp[ecx]", C},
    {fst_qax, "DD10", Z, "fst qp[eax]", A},
    {fst_qcx, "DD11", Z, "fst qp[ecx]", C},
    {fld_qbp, "DD85", "DD45", "fld qp[ebp%+d]", 5},
    {fld_qp, "DD05", Z, "fld qp[off_%p]", 4},
    {fld_sax, "D900", Z, "fld dp[eax]", A},
    {fld_sbp, "D985", "D945", "fld dp[ebp%+d]", 5},
    {fstp_sbp, "D99D", "D95D", "fstp dp[ebp%+d]", 5},
    {fstp_scx, "D919", Z, "fstp dp[ecx]", C},
    {fstp_qcx, "DD19", Z, "fstp qp[ecx]", C},
    {fstp_ssp, "D91C24", Z, "fstp dp[esp]", 0},
    {fldcw, "D92D", Z, "fldcw [off_%p]", 4},
    {fstp_qsp, "DD5C2400", Z, "fstp qp[esp]", 0},
    {fstsw, "DFE0", Z, "fstsw", 0},
    {fstp_st1, "DDD9", Z, "fstp st1", 0},
    {fst_qbp, "DD95", "DD55", "fst qp[ebp%+d]", 5},
    {fstp_qbp, "DD9D", "DD5D", "fstp qp[ebp%+d]", 5},
    {fadd_qbp, "DC85", "DC45", "fadd qp[ebp%+d]", 5},
    {fsub_qbp, "DCA5", "DC65", "fsub qp[ebp%+d]", 5},
    {fmul_qbp, "DC8D", "DC4D", "fmul qp[ebp%+d]", 5},
    {fdiv_qbp, "DCB5", "DC75", "fdiv qp[ebp%+d]", 5},
    {fadd_qp, "DC05", Z, "fadd qp[%d]", 4},
    {fsub_qp, "DC25", Z, "fsub qp[%d]", 4},
    {fmul_qp, "DC0D", Z, "fmul qp[%d]", 4},
    {fdiv_qp, "DC35", Z, "fdiv qp[%d]", 4},
    {faddp_st1_st, "DEC1", Z, "faddp st1,st", 0},
    {fmulp_st1_st, "DEC9", Z, "fmulp st1,st", 0},
    {fsubrp_st1_st, "DEE9", Z, "fsubrp st1,st", 0},
    {fdivrp_st1_st, "DEF9", Z, "fdivrp st1,st", 0},
    {fistp_dsp, Z, "DB5C24", "fistp dp[esp%+d]", 1},
    {fistp_ueax, "83EC08DF3C245883C404", Z, "fistp unsigned eax", X},
    {fild_dax, "DB00", Z, "fild dp[eax]", A},
    {fild_dsp, "DB8424", "DB4424", "fild dp[esp%+d]", 5},
    {fild_uax, "6A0050DF2C2483C408", Z, "fild unsigned eax", X},
    {fild_udsp, "6A00FF742404DF2C2483C408", Z, "fild unsigned [esp]", X},
    {fucompp, "DAE9", Z, "fucompp", 0},
    {xdiv_ecx, "99F7F9", Z, "cdq&idiv ecx", X},
    {xmod_ecx, "99F7F989D0", Z, "cdq&idiv ecx&mov eax,edx", X},
    {udiv_ecx, "31D2F7F1", Z, "xor edx,edx&div ecx", X},
    {umod_ecx, "31D2F7F189D0", Z, "xor edx,edx&div ecx&mov eax,edx", X},
    {setint, "", Z, "setint %d @%d", 0},
    {setreal, "", Z, "setreal %.3f @%d", 0},
    {setstr, "", Z, "setstr '%s' @%d", 0},
    {setaddr, "", Z, "setaddr %d @%d", 0},
    {fn_, Z, Z, "fn_%04d:", 4},
    {exp_, Z, Z, "exp fn_%04d:", 4},
    {loc_, Z, Z, "loc_%03d:", 4},
};

/* Instruction lookup */

static int cmpInst(const void *a, const void *b)
{
	return ((INSTRUCTION *)a)->opcode - ((INSTRUCTION *)b)->opcode;
}

void initInstruction(void)
{
	size_t n;
	size_t count = sizeof(x86Instructions) / sizeof(x86Instructions[0]);
	qsort(x86Instructions, count, sizeof(x86Instructions[0]), cmpInst);
	for (n = 0; n < count; n++)
	{
		INSTRUCTION *pI = &x86Instructions[n];
		if (pI->opcode != (int)n + 1)
		{
			error("INST", "mismatch! %s opcode=%d n+1=%d", pI->mnemonic, pI->opcode, (int)n + 1);
		}
	}
}

int instructionRegisters(int opcode)
{
	size_t count = sizeof(x86Instructions) / sizeof(x86Instructions[0]);
	if (opcode <= 0 || (size_t)opcode > count)
	{
		error("instruction", "opcode %d is outside the instruction table", opcode);
	}
	return x86Instructions[opcode - 1].regs_size & 0xFF00;
}

/* DLL imports */

static int fileExists(const char *path)
{
	FILE *file = fopen((char *)path, "rb");
	if (file == NULL)
	{
		return 0;
	}
	fclose(file);
	return 1;
}

static int makeLibraryPath(
    char *output, size_t capacity, const char *directory, const char *name, const char *extension)
{
	const char *dot = strrchr(name, '.');
	size_t stemLength = dot == NULL ? strlen(name) : (size_t)(dot - name);
	int written = snprintf(output,
	                       capacity,
	                       "%s%s%.*s%s",
	                       directory,
	                       *directory == '\0' ? "" : "/",
	                       (int)stemLength,
	                       name,
	                       extension);
	return written >= 0 && (size_t)written < capacity;
}

static int resolveInDirectory(const char *directory, const char *name, char *path, size_t capacity)
{
	if (makeLibraryPath(path, capacity, directory, name, ".def") && fileExists(path))
	{
		return 1;
	}
	if (makeLibraryPath(path, capacity, directory, name, ".dll") && fileExists(path))
	{
		return 1;
	}
	return 0;
}

static void resolveImportLibrary(const char *name, char *path, size_t capacity)
{
	int index;
	if (strchr(name, '/') != NULL || strchr(name, '\\') != NULL)
	{
		if (fileExists(name))
		{
			int written = snprintf(path, capacity, "%s", name);
			if (written < 0 || (size_t)written >= capacity)
			{
				error("imports", "import-library path is too long: %s", name);
			}
			return;
		}
		error("imports", "import library '%s' does not exist", name);
	}
	if (resolveInDirectory("", name, path, capacity))
	{
		return;
	}
	for (index = 0; index < cmd.libraryPathCount; ++index)
	{
		if (resolveInDirectory(cmd.libraryPaths[index], name, path, capacity))
		{
			return;
		}
	}
	if (cmd.peSysroot[0] != '\0')
	{
		char systemDirectory[MAX_PATH];
		int written;
		if (resolveInDirectory(cmd.peSysroot, name, path, capacity))
		{
			return;
		}
		written = snprintf(
		    systemDirectory, sizeof(systemDirectory), "%s/Windows/System32", cmd.peSysroot);
		if (written < 0 || (size_t)written >= sizeof(systemDirectory))
		{
			error("imports", "PE sysroot path is too long");
		}
		if (resolveInDirectory(systemDirectory, name, path, capacity))
		{
			return;
		}
	}
	if (platform_resolve_system_library(name, path, (int)capacity))
	{
		return;
	}
	error("imports", "cannot find '%s'; use -L or --pe-sysroot", name);
}

static void discoverImportLibraries(char *dllnames)
{
	int n;
	char *dllname = strtok(dllnames, ";");
	for (n = 0; dllname != NULL; dllname = strtok(NULL, ";"))
	{
		char path[MAX_PATH];
		char moduleName[MAX_PATH];
		if (n >= (int)(sizeof(exe.dll) / sizeof(exe.dll[0])))
		{
			error("imports", "too many import libraries");
		}
		resolveImportLibrary(dllname, path, sizeof(path));
		peLoadExportSymbols(path, dllname, n, moduleName, sizeof(moduleName));
		exe.dll[n].dllname = xstrdup(moduleName);
		++n;
	}
	exe.nDLL = n;
}

static void recordImport(int ixDLL, int id)
{
	int n = 0;
	if (ixDLL < 0 || ixDLL >= exe.nDLL)
	{
		error("imports",
		      "symbol '%s' has invalid import library index %d (library count %d)",
		      toString(id),
		      ixDLL,
		      exe.nDLL);
	}
	while (n < exe.dll[ixDLL].nFunc && exe.dll[ixDLL].idFunc[n] != id)
	{
		n++;
	}
	if (n >= exe.dll[ixDLL].nFunc)
	{
		if (n >= (int)(sizeof(exe.dll[ixDLL].idFunc) / sizeof(exe.dll[ixDLL].idFunc[0])))
		{
			error("imports", "too many imported functions from '%s'", exe.dll[ixDLL].dllname);
		}
		exe.dll[ixDLL].nFunc++;
		exe.dll[ixDLL].idFunc[n] = id;
	}
}

static int importLibraryIndex(int symbolId)
{
	int libraryIndex = (int)(intptr_t)cd.hash.tbl[symbolId].val & AT_ADDR;
	if (libraryIndex == AT_AMBIGUOUS_IMPORT)
	{
		error("imports",
		      "symbol '%s' is exported by more than one import library",
		      toString(symbolId));
	}
	return libraryIndex;
}

static int calculateImportTableSize(void)
{
	int k, n, size = 0;

	for (n = 0; n < exe.nDLL; n++)
	{
		if (exe.dll[n].nFunc == 0)
		{
			continue;
		}
		size += strlen(exe.dll[n].dllname) + 1;
		exe.useDLL++;
		for (k = 0; k < exe.dll[n].nFunc; k++)
		{
			size += strlen(toString(exe.dll[n].idFunc[k])) + 3;
		}
		exe.useFunc += exe.dll[n].nFunc;
	}

	size += exe.useDLL * 20 + 20;
	size += (exe.useFunc + exe.useDLL) * 4 * 2;
	size = ((size - 1) / 16 + 1) * 16;
	return size;
}

static uint8_t *buildImportTable(void)
{
	int i, n = 0, k;
	uint32_t *pImpt = xalloc((size_t)exe.lenImpt);
	int nLookup = (exe.useDLL + 1) * 5;
	int nImptAddr = nLookup + (exe.useDLL + exe.useFunc);
	int nName = (nImptAddr + (exe.useDLL + exe.useFunc)) * 4;

	for (i = 0; i < exe.nDLL; i++)
	{
		if (exe.dll[i].nFunc == 0)
		{
			continue;
		}
		pImpt[n * 5] = mem.ImptAddr + nLookup * 4;
		pImpt[n * 5 + 4] = mem.ImptAddr + nImptAddr * 4;

		for (k = 0; k < exe.dll[i].nFunc; k++)
		{
			int symbolIndex = exe.dll[i].idFunc[k];
			char *fname = toString(symbolIndex);
			cd.hash.tbl[symbolIndex].val = (void *)(intptr_t)(AT_IMPT | (pImpt[n * 5 + 4] + k * 4));
			pImpt[nLookup++] = mem.ImptAddr + nName;
			pImpt[nImptAddr++] = mem.ImptAddr + nName;
			strcpy((char *)pImpt + nName + 2, fname);
			nName += strlen(fname) + 3;
		}

		nLookup++;
		nImptAddr++;
		pImpt[n * 5 + 3] = mem.ImptAddr + nName;
		strcpy((char *)pImpt + nName, exe.dll[i].dllname);
		nName += strlen(exe.dll[i].dllname) + 1;
		n++;
	}

	return (uint8_t *)pImpt;
}

/*============================================================================
 * Export
 *============================================================================*/

typedef struct _Export
{
	int addr;
	char *name;
} Export;

static Export export[100];
static int nExport;

static int cmpName(const void *a, const void *b)
{
	return strcmp(((Export *)a)->name, ((Export *)b)->name);
}

static int calculateExportTableSize(void)
{
	int n;
	int size = strlen(cmd.outfile) + 1;
	for (n = 0; n < cd.hash.size; n++)
	{
		char *name = cd.hash.tbl[n].key;
		int val = (int)(intptr_t)cd.hash.tbl[n].val;
		if (name != NULL && (val & AT_EXPT) != 0)
		{
			if (nExport >= (int)(sizeof(export) / sizeof(export[0])))
			{
				error("exports", "too many exported functions");
			}
			export[nExport].addr = mem.CodeAddr + (val & AT_ADDR);
			export[nExport++].name = name;
			size += 10 + strlen(name) + 1;
		}
	}
	qsort(export, nExport, sizeof(Export), cmpName);
	return sizeof(IMAGE_EXPORT_DIRECTORY) + size;
}

static uint8_t *buildExportTable(void)
{
	int exportIndex;
	int directorySize = sizeof(IMAGE_EXPORT_DIRECTORY);
	int tableAddress = mem.ExptAddr + directorySize;
	char *buffer = xalloc(exe.lenExpt);
	uint32_t *addresses = (uint32_t *)(buffer + directorySize);
	uint32_t *names = (uint32_t *)(buffer + directorySize + 4 * nExport);
	uint16_t *ordinals = (uint16_t *)(buffer + directorySize + 8 * nExport);
	char *strings = buffer + directorySize + 10 * nExport;
	IMAGE_EXPORT_DIRECTORY directory = {0};

	directory.Name = tableAddress + 10 * nExport;
	directory.Base = 1;
	directory.NumberOfFunctions = nExport;
	directory.NumberOfNames = nExport;
	directory.AddressOfFunctions = tableAddress;
	directory.AddressOfNames = tableAddress + 4 * nExport;
	directory.AddressOfNameOrdinals = tableAddress + 8 * nExport;

	memcpy(buffer, &directory, sizeof(directory));

	for (exportIndex = 0; exportIndex < nExport; exportIndex++)
	{
		addresses[exportIndex] = export[exportIndex].addr;
	}

	strcpy(strings, cmd.outfile);
	strings += strlen(cmd.outfile) + 1;

	for (exportIndex = 0; exportIndex < nExport; exportIndex++)
	{
		names[exportIndex] = mem.ExptAddr + (strings - buffer);
		strcpy(strings, export[exportIndex].name);
		strings += strlen(export[exportIndex].name) + 1;
		ordinals[exportIndex] = (uint16_t)exportIndex;
	}

	return (uint8_t *)buffer;
}

/*============================================================================
 * Relocation
 *============================================================================*/

static int cmpAddr(const void *a, const void *b)
{
	return *((int *)a) - *((int *)b);
}

static void prepareRelocations(void)
{
	int n;

	qsort(exe.locs, exe.nLocs, sizeof(int), cmpAddr);
	exe.nPages = exe.nLocs > 0 ? exe.locs[exe.nLocs - 1] / 4096 : 0;
	exe.cnt = xalloc((size_t)(exe.nPages > 0 ? exe.nPages : 1) * sizeof(*exe.cnt));

	for (n = 0; n < exe.nLocs; n++)
	{
		int page = exe.locs[n] / 4096 - 1;
		if (page < 0 || page >= exe.nPages)
		{
			error("relocations", "relocation page is outside the supported image size");
		}
		exe.cnt[page]++;
	}

	mem.RelocAddr = mem.DataAddr + (mem.DataSize + MEMALIGN1) & ~MEMALIGN1;
	mem.RelocSize = 0;

	for (n = 0; n < exe.nPages; ++n)
	{
		mem.RelocSize += 8 + ((exe.cnt[n] + 1) & ~1) * 2;
	}

	raw.RelocAddr = raw.DataAddr + raw.DataSize;
	raw.RelocSize = (mem.RelocSize + RAWALIGN1) & ~RAWALIGN1;
}

static uint8_t *buildRelocationTable(void)
{
	int n, k, relocIndex = 0;
	uint8_t *buf = xalloc(raw.RelocSize);
	uint16_t *p = (uint16_t *)buf;

	for (n = 0; n < exe.nPages; n++)
	{
		int *q = (int *)p;
		*q++ = (n + 1) * 4096;
		*q++ = 8 + ((exe.cnt[n] + 1) & ~1) * 2;
		p = (uint16_t *)q;

		for (k = 0; k < exe.cnt[n]; k++)
		{
			*p++ = (uint16_t)(0x3000 + (exe.locs[relocIndex++] & 0x0FFF));
		}

		if ((exe.cnt[n] & 1) != 0)
		{
			*p++ = 0;
		}
	}

	return buf;
}

static void appendRelocation(int address)
{
	if (exe.nLocs >= exe.locCapacity)
	{
		int newCapacity = exe.locCapacity == 0 ? 1024 : exe.locCapacity * 2;
		if (newCapacity < exe.locCapacity)
		{
			error("linker", "PE relocation table is too large");
		}
		exe.locs = xrealloc(exe.locs, (size_t)newCapacity * sizeof(*exe.locs));
		exe.locCapacity = newCapacity;
	}
	exe.locs[exe.nLocs++] = address;
}

/* PE image writer */

typedef struct _EXE_HEADER
{
	IMAGE_DOS_HEADER DosHeader;
	uint8_t DosStub[64];
	uint32_t Signature;
	IMAGE_FILE_HEADER FileHeader;
	IMAGE_OPTIONAL_HEADER OptionalHeader;
	IMAGE_SECTION_HEADER SectionHeaders[2];
} EXE_HEADER;

static void writePortableExecutable(uint8_t *bufImport,
                                    uint8_t *bufExport,
                                    uint8_t *CodeBuffer,
                                    int lenCode,
                                    uint8_t *DataBuffer,
                                    int lenData)
{
	uint8_t *bufReloc = NULL;
	int sizeImptAddrTable = (exe.useDLL + exe.useFunc) * 4;
	int posImptAddrTable = mem.DataAddr + (exe.useDLL + 1) * 20 + sizeImptAddrTable;
	int numSections = mcc.typeApp == 0 ? 3 : 2;
	int typeApp = mcc.typeApp == 0 ? 2 : mcc.typeApp;
	int exptAddr = mcc.typeApp == 0 ? mem.ExptAddr : 0;
	EXE_HEADER exeHeader = {
	    {
	        0x5A4D, 0x90, 3,
	        0,      4,    0,
	        0xFFFF, 0,    0xB8,
	        0,      0,    0,
	        0x40,   0,    {0, 0, 0, 0},
	        0,      0,    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	        0x0080,
	    },
	    "\x0E\x1F\xBA\x0E\x00\xB4\x09\xCD\x21\xB8\x01\x4C\xCD\x21"
	    "This program cannot be run in DOS mode.\r\r\n$",
	    0x00004550,
	    {0x014c, (uint16_t)numSections, 0, 0, 0, (uint16_t)sizeof(IMAGE_OPTIONAL_HEADER), 0x030F},
	    {0x010B,
	     0x06,
	     0x00,
	     0,
	     0,
	     0,
	     exe.entryPoint,
	     mem.CodeAddr,
	     mem.DataAddr,
	     exe.base,
	     MEMALIGN,
	     RAWALIGN,
	     4,
	     0,
	     0,
	     0,
	     4,
	     0,
	     0,
	     exe.sizeImage,
	     RAWALIGN,
	     0,
	     (uint16_t)typeApp,
	     0,
	     0x100000,
	     0x1000,
	     0x100000,
	     0x1000,
	     0,
	     0x10,
	     {{exptAddr, exe.lenExpt},
	      {mem.ImptAddr, (exe.useDLL + 1) * 20},
	      {0, 0},
	      {0, 0},
	      {0, 0},
	      {0, 0},
	      {0, 0},
	      {0, 0},
	      {0, 0},
	      {0, 0},
	      {0, 0},
	      {0, 0},
	      {posImptAddrTable, sizeImptAddrTable},
	      {0, 0},
	      {0, 0},
	      {0, 0}}},
	    {{".text",
	      {mem.CodeSize},
	      mem.CodeAddr,
	      raw.CodeSize,
	      raw.CodeAddr,
	      0,
	      0,
	      0,
	      0,
	      0x60000020},
	     {".data",
	      {mem.DataSize},
	      mem.DataAddr,
	      raw.DataSize,
	      raw.DataAddr,
	      0,
	      0,
	      0,
	      0,
	      0xC0000040}}};
	IMAGE_SECTION_HEADER reloc = {".reloc",
	                              {mem.RelocSize},
	                              mem.RelocAddr,
	                              raw.RelocSize,
	                              raw.RelocAddr,
	                              0,
	                              0,
	                              0,
	                              0,
	                              0x42000040};

	uint8_t *bufEXE = xalloc(raw.DataAddr + raw.DataSize);
	exeHeader.OptionalHeader.SizeOfCode = (uint32_t)raw.CodeSize;
	exeHeader.OptionalHeader.SizeOfInitializedData = (uint32_t)raw.DataSize;
	if (exe.useDLL == 0)
	{
		exeHeader.OptionalHeader.DataDirectory[1].VirtualAddress = 0;
		exeHeader.OptionalHeader.DataDirectory[1].Size = 0;
		exeHeader.OptionalHeader.DataDirectory[12].VirtualAddress = 0;
		exeHeader.OptionalHeader.DataDirectory[12].Size = 0;
	}
	if (mcc.typeApp == 0)
	{
		exeHeader.FileHeader.Characteristics |= 0x2000U;
		if (mem.RelocSize != 0)
		{
			exeHeader.FileHeader.Characteristics &= 0xFFFEU;
			exeHeader.OptionalHeader.DataDirectory[5].VirtualAddress = (uint32_t)mem.RelocAddr;
			exeHeader.OptionalHeader.DataDirectory[5].Size = (uint32_t)mem.RelocSize;
		}
	}
	memcpy(bufEXE, &exeHeader, sizeof(exeHeader));
	if (mcc.typeApp == 0)
	{
		memcpy(bufEXE + sizeof(exeHeader), &reloc, sizeof(reloc));
		bufReloc = buildRelocationTable();
	}
	memcpy(bufEXE + RAWALIGN, CodeBuffer, lenCode);
	memcpy(bufEXE + raw.ImptAddr, bufImport, exe.lenImpt);
	if (bufExport != NULL)
	{
		memcpy(bufEXE + raw.ImptAddr + exe.lenImpt, bufExport, exe.lenExpt);
	}
	memcpy(bufEXE + raw.ImptAddr + exe.lenImpt + exe.lenExpt, DataBuffer, lenData);
	FILE *fpDst = fopen(cmd.outfile, "wb");
	if (fpDst == NULL)
	{
		error("PE writer", "cannot open '%s'", cmd.outfile);
	}
	if (fwrite(bufEXE, raw.DataAddr + raw.DataSize, 1, fpDst) != 1)
	{
		error("PE writer", "cannot write '%s'", cmd.outfile);
	}
	if (bufReloc != NULL && fwrite(bufReloc, raw.RelocSize, 1, fpDst) != 1)
	{
		error("PE writer", "cannot write relocations to '%s'", cmd.outfile);
	}
	if (fclose(fpDst) != 0)
	{
		error("PE writer", "cannot close '%s'", cmd.outfile);
	}
	free(bufReloc);
	free(bufEXE);
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

static void resolveDefinedDataSymbols(void)
{
	int index;
	for (index = 0; index < ix.ixCode; ++index)
	{
		INSTRUCT *item = &cd.pCode[index];
		Name *symbol;
		if (item->attr != AD_IMPORT)
		{
			continue;
		}
		symbol = getNameFromTable(globTable, NM_VAR, item->num);
		if (symbol == NULL || symbol->addrType == AD_IMPORT)
		{
			continue;
		}
		item->attr = symbol->addrType;
		item->num = symbol->address;
	}
}

static int setOffset(int *pLoc, int *pOffCode, int cnt)
{
	int n, inst, num, attr, size, nChg = 0;
	int offCode = 0;
	for (n = 0; n < ix.ixCode; n++)
	{
		instAt(n, &inst, &num, &attr, &size);
		INSTRUCTION *pI = &x86Instructions[inst - 1];
		char *fmt = pI->mnemonic;
		if ((opt & oASM) && cnt == 0)
		{
			printf((strchr(fmt, ':') != NULL ? "%4d: " : "%4d:     "), n);
			if (inst == setreal)
			{
				printf(fmt, cd.pCode[n].dval, cd.pCode[n].offset);
			}
			else if (inst == setstr)
			{
				printf(fmt, cd.pCode[n].sval, cd.pCode[n].offset);
			}
			else
			{
				printf(fmt, num, cd.pCode[n].offset);
			}
			if (fmt[0] == 'j')
			{
				printf(attr == AD_CONST ? "%d" : "loc_%03d", num);
			}
			else if (inst == xret && num > 0)
			{
				printf(" %d", num);
			}
			else if (attr != 0)
			{
				printf("[%d]", attr);
			}
			if (pI->opcode == fn_ || pI->opcode == call)
			{
				printf("[%s]", cd.hash.tbl[num].key);
			}
			printf("\n");
		}
		if (setint <= inst && inst <= setaddr)
		{
			continue;
		}
		if (cnt == 0)
		{
			cd.pCode[n].size = 0;
		}
		cd.pCode[n].offset = offCode;
		if (pI->opcode == loc_ && num <= ix.ixLoc)
		{
			pLoc[num] = offCode;
		}
		else if (pI->opcode == fn_ || pI->opcode == exp_)
		{
			if (strcmp(cd.hash.tbl[num].key, "_main") == 0)
			{
				exe.entryPoint = offCode;
			}
			int at = ((int)(intptr_t)cd.hash.tbl[num].val) & 0xFF000000;
			cd.hash.tbl[num].val = (void *)(intptr_t)(at + offCode);
		}
		else if (pI->mnemonic[0] == 'j')
		{
			int fByte = abs(pLoc[num] - offCode - 2) < 128;
			int fS = (opt & oUOPT) ? (pLoc[num] > 0 && fByte) : (cnt == 0 || fByte);
			fS = (fS || attr == AD_CONST);
			offCode += strlen(fS ? pI->hexcode2 : pI->hexcode) / 2 + (fS ? 1 : 4);
			int sizeOld = cd.pCode[n].size;
			cd.pCode[n].size = fS ? 1 : 4;
			if (cd.pCode[n].size != sizeOld)
			{
				nChg++;
			}
		}
		else if (pI->opcode == call)
		{
			int importValue = (int)(intptr_t)cd.hash.tbl[num].val;
			int fImpt = importValue & AT_IMPT;
			offCode += fImpt ? 6 : 5;
			if (fImpt && cnt == 0)
			{
				recordImport(importLibraryIndex(num), num);
			}
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
		{
			recordImport(importLibraryIndex(num), num);
		}
	}
	*pOffCode = offCode;
	return nChg;
}

void link(void)
{
	int n, inst, num, attr, size, offset, offCode, cnt = 0, nChg;
	char *p, *hexcode, *caText;

	int *pLoc = xalloc((ix.ixLoc + 1) * sizeof(int));
	resolveDefinedDataSymbols();
	discoverImportLibraries(cmd.impfiles);
	do
	{
		nChg = setOffset(pLoc, &offCode, cnt++);
	} while (!(opt & oUOPT) && nChg > 0);

	mem.CodeAddr = MEMALIGN;
	mem.CodeSize = offCode;
	raw.CodeAddr = RAWALIGN;
	raw.CodeSize = (offCode + RAWALIGN1) & ~RAWALIGN1;

	exe.lenImpt = calculateImportTableSize();
	exe.lenExpt = (opt & oDLL) ? calculateExportTableSize() : 0;
	int posData = exe.lenImpt + exe.lenExpt;

	mem.DataAddr = mem.CodeAddr + (offCode + MEMALIGN1) & ~MEMALIGN1;
	mem.DataSize = exe.lenImpt + exe.lenExpt + ix.ixData + ix.ixZero;

	raw.DataAddr = raw.CodeAddr + raw.CodeSize;
	raw.DataSize = (exe.lenImpt + exe.lenExpt + ix.ixData + RAWALIGN1) & ~RAWALIGN1;
	mem.ImptAddr = mem.DataAddr;
	raw.ImptAddr = raw.DataAddr;
	mem.ExptAddr = mem.ImptAddr + exe.lenImpt;
	raw.ExptAddr = raw.ImptAddr + exe.lenImpt;

	exe.base = (opt & oDLL) ? DLLBASE : IMAGEBASE;
	exe.entryPoint += mem.CodeAddr;
	uint8_t *bufImport = buildImportTable();
	uint8_t *bufExport = (opt & oDLL) ? buildExportTable() : NULL;
	p = caText = xalloc(raw.CodeSize);
	int dataAddr = exe.base + mem.DataAddr + posData;
	for (n = 0; n < ix.ixCode; n++)
	{
		uint8_t *q;
		instAt(n, &inst, &num, &attr, &size);
		offset = cd.pCode[n].offset;
		if (inst >= setint && inst <= setaddr)
		{
			continue;
		}
		INSTRUCTION *pI = &x86Instructions[inst - 1];
		hexcode = size == 1 || (inst == xret && num > 0) ? pI->hexcode2 : pI->hexcode;
		if (hexcode == NULL)
		{
			continue;
		}
		int fReloc = TRUE;
		if (pI->opcode == call)
		{
			int fImpt = (int)(intptr_t)cd.hash.tbl[num].val & AT_IMPT;
			int fUser = (int)(intptr_t)cd.hash.tbl[num].val & AT_USER;
			int addr = (int)(intptr_t)cd.hash.tbl[num].val & AT_ADDR;
			if (!fImpt && !fUser)
			{
				if (num < 0)
				{
					error("linker", "internal function symbol %d was not mapped", num);
				}
				error("linker", "'%s' undeclared", toString(num));
			}
			num = fImpt ? (exe.base + addr) : (addr - cd.pCode[n + 1].offset);
			hexcode = fImpt ? "FF15" : "E8";
			if (!fImpt)
			{
				fReloc = FALSE;
			}
			size = 4;
		}
		else if (pI->mnemonic[0] == 'j')
		{
			int sizeInst = size == 1 ? 2 : inst == jmp ? 5 : 6;
			if (attr != AD_CONST)
			{
				num = pLoc[num] - (cd.pCode[n].offset + sizeInst);
			}
			fReloc = FALSE;
		}
		else if (attr == AD_CODE)
		{
			int foffset = (int)(intptr_t)cd.hash.tbl[num].val & AT_ADDR;
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
			num = exe.base + ((int)(intptr_t)cd.hash.tbl[num].val & AT_ADDR);
		}
		else
		{
			fReloc = FALSE;
		}
		for (q = (uint8_t *)hexcode; *q != '\0'; q += 2)
		{
			*p++ = (uint8_t)((hexDigitValue(*q) << 4) + hexDigitValue(q[1]));
		}
		if (size == 1)
		{
			*p = (uint8_t)num;
		}
		else if (size == 2)
		{
			writeU16Le((unsigned char *)p, (unsigned int)num);
		}
		else if (size == 4)
		{
			writeU32Le((unsigned char *)p, (unsigned int)num);
		}
		if (fReloc && (opt & oDLL))
		{
			appendRelocation(mem.CodeAddr + (int)(p - caText));
		}
		p += size;
	}
	char *caData = xalloc((size_t)ix.ixData + 1U);
	for (n = 0; n < ix.ixCode; n++)
	{
		instAt(n, &inst, &num, &attr, &size);
		offset = cd.pCode[n].offset;
		if (inst < setint || setaddr < inst)
		{
			continue;
		}
		if (inst == setint && attr == 1)
		{
			caData[offset] = (char)num;
		}
		else if (inst == setint && attr == 2)
		{
			writeU16Le((unsigned char *)&caData[offset], (unsigned int)num);
		}
		else if (inst == setint && attr == 4)
		{
			writeU32Le((unsigned char *)&caData[offset], (unsigned int)num);
		}
		else if (inst == setint)
		{
			writeU32Le((unsigned char *)&caData[offset], (unsigned int)(num + dataAddr));
		}
		else if (inst == setreal)
		{
			writeDoubleLe((unsigned char *)&caData[offset], cd.pCode[n].dval);
		}
		else if (inst == setstr)
		{
			decodeString(&caData[offset], cd.pCode[n].sval);
		}
		else
		{
			int value;

			if (attr == AD_CODE)
			{
				value = exe.base + mem.CodeAddr + ((int)(intptr_t)cd.hash.tbl[num].val & AT_ADDR);
			}
			else if (attr == AD_DATA)
			{
				value = dataAddr + num;
			}
			else if (attr == AD_IMPORT)
			{
				value = exe.base + ((int)(intptr_t)cd.hash.tbl[num].val & AT_ADDR);
			}
			else
			{
				error("linker", "invalid data relocation type %d", attr);
			}
			value += cd.pCode[n].refs;
			writeU32Le((unsigned char *)&caData[offset], (unsigned int)value);
			if (opt & oDLL)
			{
				appendRelocation(mem.DataAddr + posData + offset);
			}
		}
	}
	if (opt & oDLL)
	{
		prepareRelocations();
		exe.sizeImage = mem.RelocAddr + (mem.RelocSize + MEMALIGN1) & ~MEMALIGN1;
	}
	else
	{
		exe.sizeImage = mem.DataAddr + (mem.DataSize + MEMALIGN1) & ~MEMALIGN1;
	}
	writePortableExecutable(
	    bufImport, bufExport, (uint8_t *)caText, offCode, (uint8_t *)caData, ix.ixData);
	free(exe.cnt);
	free(exe.locs);
	free(caData);
	free(caText);
	free(bufExport);
	free(bufImport);
	free(pLoc);
	for (n = 0; n < exe.nDLL; ++n)
	{
		free(exe.dll[n].dllname);
	}
}
