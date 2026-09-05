# IR, ABI, and backend corrections

This change builds on the review of revision `4c352e9`. It implements the reproduced frontend fixes and introduces PowerPC32 Linux and AIX linkage policies. It does not certify native AIX or z/OS output.

## Corrected behavior

| Area | Implementation and regression coverage |
| --- | --- |
| C semantics | Local string initialization enters the authoritative IR; symbolic global character pointers retain relocations; parenthesized array indexing retains object metadata; unsigned shift counts do not change the signedness of the promoted left operand. |
| Floating point | Binary32 and binary64 have distinct storage and conversions. Logical operands retain their source types until conversion. Unary operations use integer promotions. Tests include exact single-precision storage and NaN comparisons. |
| IR verification | Operand/result types, widths, definitions, dominance, terminators, variadic state, storage counts, and global relocation bounds are checked. Negative tests require a diagnostic exit, not a crash. |
| Variadic calls | `va_start`, `va_arg`, `va_copy`, and `va_end` are explicit IR operations. Each backend traverses its actual incoming argument representation. |
| Aggregates | Supported small aggregate values keep an aggregate IR kind, allowing hidden result pointers, Linux indirect arguments, and AIX/z/OS left-justified argument words. |
| Windows PE | stdcall returns pop argument bytes. DLL headers identify DLLs and publish base relocations. Empty imports have absent directories. Large frames probe each stack page. |
| HLASM addressing | Nonzero address bases, materialized frame offsets, aligned independently addressed globals, long branches, and explicit literal placement replace invalid addressing assumptions. |
| HLASM data/linkage | Exact IEEE bits replace HFP literals. Ordered floating comparisons exclude unordered results. Calls use the non-XPLINK C word image and hidden floating/aggregate return storage. Zero storage is initialized; C newline is X'15'. |
| HLASM module identity | Translation-unit-qualified aliases distinguish writable sections and private function entries; static definitions and references share symbol identity. Prologs specify AMODE 31 and close previous USING scopes. |
| PowerPC | Shared scalar instruction selection and spill layout; separate System V and AIX argument classifiers, calls, symbols, descriptors, and TOC handling. AIX member alignment and unsigned plain `char` follow target policy. |
| Host correctness | Strict Clang/Linux builds exposed missing initializers, pointer-width casts, POSIX declarations, and leaked parser metadata. These were fixed without suppressing the warnings or disabling leak checks. |

The Windows large-frame regression originally faulted after jumping over a guard page. The new prolog walks the frame in page-sized steps, consistent with [Microsoft's stack-probe requirements](https://learn.microsoft.com/en-us/cpp/preprocessor/check-stack?view=msvc-170).

The HLASM corrections were checked against IBM's assembler and Language Environment references, linked in [z/OS validation](zos-validation.md). One initial review concern required qualification: `CEEENTRY MAIN=YES` can establish the C environment. The added IBM-compiled companion specifically selects BFP mode; it is not a replacement process-entry wrapper.

## Evidence and remaining boundaries

Windows tests execute generated PE images, load a relocated DLL, and bootstrap through stage 3. Linux tests assemble and inspect ELF32 objects and run PowerPC code under QEMU. The Clang interoperability test exercises calls in both directions. Linux host tests also run with address/undefined-behavior sanitizers and leak detection.

The September 4, 2026 validation run passed all 255 Windows tests and all 204 Linux sanitizer tests. Windows used MSVC with `/W4 /WX`, LLVM 18.1.8, and QEMU 10.0.11 through Debian WSL. Linux used Clang 19.1.7 with warnings as errors and non-recovering undefined-behavior checks. These counts include structural output tests and must not be read as counts of native AIX/z/OS executions.

AIX tests check assembly structure, not native XCOFF assembly or execution. HLASM tests check record constraints and selected linkage/addressing invariants, not macro expansion or native execution. The acceptance procedures remain [PowerPC validation](ppc-validation.md) and [z/OS validation](zos-validation.md).

The final HLASM and PPC backends consume typed IR directly. Some legacy frontend instruction emission still coexists with IR construction in the parser; removing that remaining machinery is a separate architectural cleanup, not something this change claims to have completed. The IR is not yet an optimizing SSA pipeline with register allocation, nor a complete representation of every C aggregate or callable-signature feature. Unsupported language/target modes remain explicit limits described in the README.
