# CC

CC is a compact C compiler with native code generation for 32-bit Windows and an HLASM backend for IBM z/OS. The project favors predictable compilation, explicit diagnostics, and small, auditable implementation layers over broad but incomplete language coverage.

The compiler is actively validated through native execution, cross-backend output checks, sanitizers, and static analysis. Source warnings are errors by default, and an unsupported construct must stop compilation instead of producing partial code.

## Highlights

- Emits PE32 executables and DLLs for 32-bit Windows.
- Emits HLASM 1.6 source for z/OS 2.5 using AMODE 31, RMODE ANY, and the non-XPLINK Language Environment convention.
- Uses an architecture-neutral typed control-flow IR shared by independent target backends.
- Supports separate translation units and external symbol resolution.
- Includes object-like, function-like, and variadic macros; `#`, `##`, conditional directives, logical lines, and short-circuit preprocessor expressions.
- Treats compiler warnings as errors throughout the host build.

## Build and test

Use an x86 Developer PowerShell for Visual Studio on Windows:

```powershell
cmake --preset windows-x86-debug
cmake --build --preset windows-x86-debug
ctest --preset windows-x86-debug -j 4
```

On Linux, macOS, or another environment with Ninja available:

```sh
cmake --preset portable-debug
cmake --build --preset portable-debug
ctest --preset portable-debug
```

Configure with `CC_ENABLE_SANITIZERS=ON` to enable AddressSanitizer. GNU and Clang toolchains also enable UndefinedBehaviorSanitizer.

## Usage

```text
cc [options] source.c

--target=x86-pe             Generate a 32-bit Windows PE image
--target=zos-hlasm          Generate HLASM source for z/OS
-hlasm                     Alias for --target=zos-hlasm
-S                         Emit assembly for the HLASM target
-o file                    Select the output file
-Dname[=value]             Define a preprocessor macro
-Werror                    Treat source warnings as errors (the default)
-Wno-error                 Report source warnings without stopping compilation
--exec-charset=ibm-1047    Use IBM-1047 as the z/OS execution character set
--exec-charset=ibm-037     Use IBM-037 as the z/OS execution character set
--zos-jcl=file.jcl         Generate an assemble, bind, and run job
```

For example:

```powershell
cc --target=zos-hlasm -S hello.c -o hello.asm --zos-jcl=hello.jcl
```

Each z/OS invocation compiles exactly one translation unit. Compile every `.c` file separately and pass the resulting objects to the binder. The generated self-contained JCL is intended to validate one translation unit containing `main`; it uses `CEE.SCEEMAC`, `CEE.SCEELKED`, and GOFF, and prevents bind or execution steps from running after a nonzero return code.

## Windows x86 target

The native backend writes PE32 images directly and does not depend on a system assembler or linker. It supports executables, DLL imports and exports, relocations, static data, and floating-point operations.

CC is self-hosting on the Windows x86 target. The bootstrap test builds a stage-2 compiler, uses it to recompile the complete compiler as stage 3, then uses stage 3 to compile and run an x86 program and generate z/OS HLASM. Target selection uses function-pointer descriptors, while both backends consume the same typed CFG IR and symbolic global relocations.

## z/OS HLASM target

The z/OS backend follows a defined ABI and storage model:

- `CEEENTRY`, `CEETERM`, `CEEPPA`, `CEEDSA`, and `CEECAA` provide the Language Environment entry and exit contract.
- Each function has its own DSA for automatic objects, typed IR values, BFP temporaries, and argument lists. R13 remains the DSA register and is never repurposed as an expression stack.
- Instructions are emitted in an `RSECT`; writable C objects are emitted in the `CCDATA CSECT`.
- Non-XPLINK calls pass a list of argument pointers in R1 and set the VL bit on the final entry.
- Integer values return in R15 and `double` values return in F0.
- The `PLIST(HOST)` entry is converted to `argc` and `argv`, including quoted arguments and the required `argv[argc] == NULL` terminator.
- Long relative branches and immediate materialization do not rely on a single literal pool with a 4 KiB reach.
- Character and string literals are converted to IBM-1047 or IBM-037, while numeric escapes retain their explicit values. Numeric data is emitted in big-endian order.
- C external names are preserved with `ALIAS`; generated HLASM symbols remain within the eight-character limit.
- An IR opcode without a valid lowering is a compilation error. The backend never emits placeholder assembly.

Generated modules are linked as `NORENT` because writable C globals have static storage duration and remain in the load module. Function code and automatic storage are reentrant, and executable sections are read-only. Declaring the complete module `RENT` without moving writable globals to WSA would be incorrect.

## Language support

The frontend supports `_Bool`; signed and unsigned `char`, `short`, `int`, and `long` in the ILP32 model; `float`; `double`; pointers; function pointers; arrays; functions; `struct`; `union`; and `enum`. Both backends preserve integer promotions, signed and unsigned comparisons, division, remainder, shifts, and conversions.

Scalar and pointer typedefs, first-level designated initializers, and zero initialization of omitted members are supported. The regression suite also covers declarations in `for` initializers, unnamed prototype parameters, `stdarg`, chained casts, unevaluated `sizeof` operands, and `<<=` and `>>=`.

The current IR and grammar intentionally do not implement `long long`, bit-fields, chained designators, compound literals, or variable-length arrays. These constructs produce diagnostics instead of incomplete output.

## Validation

The test suite executes programs produced by the x86 backend, checks their exit codes, bootstraps the compiler, and validates every generated HLASM and JCL artifact structurally. The project is also built with strict host warnings, AddressSanitizer, and MSVC static analysis.

Final acceptance of z/OS output requires a real HLASM listing and binder result. The mainframe procedure and expected checks are described in [docs/zos-validation.md](docs/zos-validation.md).

## License

CC is released under the Unlicense. See [LICENSE](LICENSE).
