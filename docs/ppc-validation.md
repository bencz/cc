# PowerPC32 validation

CC emits assembly for two big-endian, hard-float, ILP32 ABIs. ELF32 and XCOFF32 are object formats produced by the downstream assembler, not binary formats written by CC.

## Linux automation

Configure with LLVM tools on `PATH`, or supply `CC_LLVM_MC`, `CC_LLVM_READOBJ`, `CC_LLVM_LLD`, and `CC_LLVM_CLANG` explicitly. CMake also searches `C:/llvm/bin` on Windows.

```sh
cmake --preset portable-debug -DCC_PPC_QEMU=/usr/bin/qemu-ppc
cmake --build --preset portable-debug
ctest --preset portable-debug --output-on-failure -R ppc32
```

Windows can use native LLVM and QEMU inside Debian WSL:

```powershell
cmake --preset windows-x86-debug -DCC_PPC_USE_WSL=ON -DCC_PPC_QEMU=/usr/bin/qemu-ppc
```

The QEMU path is a Linux path when WSL is enabled. CMake does not install tools. These options also accept a privately extracted `qemu-user` executable.

Generation tests assemble Linux source with `llvm-mc` and inspect the result with `llvm-readobj`. The object must identify as ELF32 PowerPC, big-endian. Execution tests link a freestanding process entry with `ld.lld` and run the image under QEMU. Tests requiring libc are excluded from this freestanding group; exit status is checked modulo 256, as required by the Linux process interface.

`ppc32-linux.interop.clang` links a CC translation unit with a Clang translation unit using Clang's own `stdarg.h`. It checks small-structure argument copies and hidden returns, a callback, unsigned plain `char`, and mixed integer/double variadic calls in both directions beyond the argument registers. This is stronger evidence than agreement between two CC-generated functions, but it does not certify untested ABI features.

## ABI contracts

The Linux classifier uses R3–R10 and F1–F8 independently, 16-byte stack alignment, and a register-save/overflow-area variadic state. See the [PowerPC System V ABI supplement](https://www.uclibc.org/docs/psABI-ppc.pdf). Aggregate returns use a hidden destination pointer according to the [Linux PPC32 ABI requirements](https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-PPC32/LSB-Core-PPC32.html).

AIX has a separate classifier: R3–R10 shadow the parameter word image, floating arguments use F1–F13, and the caller reserves its linkage and argument areas. See [IBM's argument-passing convention](https://www.ibm.com/docs/en/aix/7.3.0?topic=sequence-argument-passing). AIX function addresses refer to descriptors containing the entry address, TOC, and environment; indirect calls load those fields and preserve the caller's TOC. See the [IBM XCOFF specification](https://www.ibm.com/docs/ssw_aix_72/filesreference/XCOFF.html).

AIX POWER layout limits subsequent aggregate-member alignment while preserving the preferred alignment of a leading double or leading aggregate containing one. Unions treat every member as a first member. This differs from Linux natural layout; see [IBM aggregate alignment](https://www.ibm.com/docs/en/xl-c-aix/13.1.0?topic=modes-alignment-aggregates) and the [LLVM implementation review](https://reviews.llvm.org/D79719).

## Native AIX acceptance

1. Generate one source per translation unit with `cc --target=ppc32-aix -S source.c -o source.s`.
2. Assemble with the AIX assembler in 32-bit mode, for example `as -a32 -o source.o source.s`, requiring zero warnings and errors.
3. Inspect XCOFF symbols, csects, relocation records, descriptors, TOC entries, and traceback tables using the installed AIX object tools.
4. Compile reference C with IBM XL/Open XL in 32-bit, default POWER-alignment mode. Link through the C compiler driver so startup and runtime libraries are present.
5. Execute the scalar, pointer, floating-point, aggregate, and varargs regressions. Test direct calls and descriptor-based callbacks in both directions, including external functions that change R2.
6. Link separate translation units with equally named private functions and data. Require the expected private symbols and exported definitions in the linker map.

The local AIX tests check emitted structure only. LLVM 18.1.8's AIX assembly parser rejects XCOFF directives used by this output; an LLVM-generated reference object is not evidence that CC's assembly has assembled. Native AIX assembly, linking, and execution have not been performed in this workspace.

Both backends deliberately use a spill-based implementation. Supported aggregate values are 1, 2, or 4 bytes; larger aggregate objects are accessed by address. PIC, shared-library output, soft-float, vector types, 64-bit integer values, and PPC64 are not implemented target modes.
