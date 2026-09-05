# CC

CC is a compact, self-hosting C compiler with independent backends for Windows x86, IBM z/OS, and big-endian PowerPC32. Its focus is a readable implementation, explicit ABI contracts, and predictable diagnostics.

C expressions and control flow are lowered to a shared typed IR. Target descriptors select the data model and output services; the two PowerPC ABIs share instruction selection and use separate linkage policies. Source warnings are errors by default, and host builds also treat warnings as errors.

## Targets

| Target | Output | Calling convention |
| --- | --- | --- |
| `x86-pe` | PE32 executable or DLL | Windows x86 cdecl / stdcall |
| `zos-hlasm` | HLASM source | z/OS AMODE 31, LE non-XPLINK C |
| `ppc32-linux` | GNU-style assembly for ELF32 | Linux PowerPC System V, big-endian |
| `ppc32-aix` | AIX assembly for XCOFF32 | AIX linkage, TOC and function descriptors |

The Windows backend writes the image directly. PowerPC and z/OS output must be assembled and linked with the target toolchain; CC does not write ELF, XCOFF, or GOFF object files directly.

## Build and test

In an x86 Visual Studio developer shell:

```powershell
cmake --preset windows-x86-debug
cmake --build --preset windows-x86-debug
ctest --preset windows-x86-debug -j 4
```

On Linux or macOS, with a C99 compiler and Ninja:

```sh
cmake --preset portable-debug
cmake --build --preset portable-debug
ctest --preset portable-debug
```

Configure with `-DCC_ENABLE_SANITIZERS=ON` to enable AddressSanitizer; GNU and Clang builds also enable UndefinedBehaviorSanitizer. PowerPC assembly checks use `llvm-mc` and `llvm-readobj` when available. Optional execution and ABI interoperability tests additionally use `ld.lld`, Clang, and `qemu-ppc`; see [PowerPC validation](docs/ppc-validation.md).

## Usage

```text
cc --target=x86-pe program.c -o program.exe
cc --target=zos-hlasm -S program.c -o program.asm --zos-jcl=program.jcl
cc --target=ppc32-linux -S program.c -o program-linux.s
cc --target=ppc32-aix -S program.c -o program-aix.s
```

The default target is `x86-pe`. Useful options include `-Dname[=value]`, `-o file`, `-shared` for Windows DLLs, and `--emit-ir=file` for IR inspection. `-Werror` is the default; `-Wno-error` explicitly relaxes source warnings.

Each PowerPC or z/OS invocation accepts one translation unit. Compile each source separately, then combine its object with the other objects and runtime libraries using the target linker. Windows can compile multiple source files into one image.

## Windows and self-hosting

The x86 backend supports imports, exports, base relocations, static data, integer operations, and binary32/binary64 floating point. The DLL regression test loads an image away from its preferred base and calls an exported stdcall function.

The bootstrap builds a stage-2 compiler, uses stage 2 to compile the complete compiler into stage 3, then exercises stage 3 with executable regression cases and output from the HLASM and both PowerPC targets. Self-hosting currently refers to Windows x86; it is not a claim of a native AIX or z/OS bootstrap.

## PowerPC32

Both targets use a big-endian ILP32 data model and hardware floating point. The backend handles scalar operations, memory access, branches, direct and indirect calls, static initializers, symbolic relocations, and explicit variadic IR operations.

Linux uses independent integer and floating-point argument registers and its register-save/overflow-area `va_list`. AIX uses its parameter word image, 24-byte linkage area, TOC preservation, and three-word function descriptors. AIX POWER aggregate alignment is selected through the target data-layout policy.

Linux assembly is assembled to ELF32 and executed under QEMU in the optional test suite. Tests also link CC-generated code with Clang-generated code to exercise calls in both directions. AIX assembly has structural checks, but native assembly, binding, and execution remain required acceptance gates.

## z/OS

The HLASM backend consumes the same IR directly; it does not translate x86 instructions. It uses LE entry/exit macros, per-function automatic storage, long relative branches, materialized stack addresses, and exact IEEE floating-point constants.

Non-XPLINK C calls use a word-oriented argument area addressed by R1. Integer and pointer results use R15; floating-point and supported aggregate results use caller-provided storage through a hidden argument. This is not the OS-linkage list-of-pointers convention.

Generated C literals use IBM-1047 by default, or IBM-037 with `--exec-charset=ibm-037`. Numeric escapes keep their specified byte values. Plain `char` is unsigned on z/OS and PowerPC, and signed on Windows x86.

A translation unit containing `main` also produces `program.asm.runtime.c`: a small, complete IEEE-mode initializer compiled with the IBM C compiler and its headers. The generated JCL includes this compilation step before binding. Library-only units rely on the caller's compatible LE/BFP environment.

Modules with writable globals are bound as `NORENT`. Mainframe acceptance requires an actual IBM HLASM listing, binder map, and execution; local text checks do not establish those results. See [z/OS validation](docs/zos-validation.md).

## Language scope

The frontend supports ILP32 integer types, `_Bool`, `float`, `double`, pointers, function pointers, arrays, functions, structures, unions, enums, typedefs, designated initializers, and variadic functions. The preprocessor includes function-like and variadic macros, stringification, token pasting, conditional directives, and short-circuit expressions.

Aggregate objects can be larger than a machine word and accessed through pointers. Aggregate values passed or returned by value are currently limited to objects of 1, 2, or 4 bytes. The compiler does not yet support `long long`, bit-fields, variable-length arrays, compound literals, or chained designators. PowerPC output is non-PIC; shared-library generation and PPC64 are outside the current target contract.

## License

CC is released under the Unlicense. See [LICENSE](LICENSE).
