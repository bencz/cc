# Validation on z/OS

Local generation checks are not a substitute for IBM HLASM, the binder, and execution under Language Environment. The target contract is AMODE 31, RMODE ANY, non-XPLINK C linkage, IEEE binary floating point, and NORENT modules.

## Build the generated program

1. Generate assembly and JCL: `cc --target=zos-hlasm -S program.c -o program.asm --zos-jcl=program.jcl`.
2. Transfer the JCL as text using the installation's expected encoding. Literal bytes are emitted in hexadecimal; select `--exec-charset=ibm-037` if the program's execution character set is IBM-037 instead of the default IBM-1047.
3. Adapt the JOB card, accounting, classes, and installed data-set prefixes. The defaults name `CBC.SCCNCMP`, `CEE.SCEEMAC`, `CEE.SCEEH.H`, `CEE.SCEELKED`, `CEE.SCEELKEX`, `CEE.SCEERUN`, and `CEE.SCEERUN2`; these are site assumptions, not universal locations.
4. Submit the job. Require RC=0 from `ASM`, `IEEE`, and `BIND`, then the expected program result from `RUN`. Nonzero earlier results suppress later steps, including assembler/compiler warnings.
5. Retain the expanded assembler listing, compiler listing, binder map, program output, and any LE dump as acceptance evidence.

The `IEEE` step compiles a complete companion function with `CCNDRVR`, the IBM headers, and `FLOAT(IEEE),NOXPLINK,NORENT,LONGNAME`. It calls `__fp_setmode(_FP_BFP_MODE)` before the generated main calls user code. The same source is written to `program.asm.runtime.c` for builds not using generated JCL. This requires an installed IBM C compiler; it avoids inventing numeric constants from its private headers. See [IBM's floating-point mode interface](https://www.ibm.com/docs/en/zos/3.1.0?topic=functions-fp-setmode-set-ieee-hexadecimal-mode).

`CEEENTRY MAIN=YES` establishes the LE/C environment; a separate C process-entry wrapper is not required solely to establish that environment. See [IBM: establishing the C/C++ environment](https://www.ibm.com/docs/en/zos/3.2.0?topic=programs-establishing-zos-xl-cc-environment).

## Inspect the listing and binder map

Check every generated CSECT and external alias, including translation-unit-qualified code/data identities and private function entries. No intended section may disappear through duplicate-section selection. Public C names retain their external aliases.

Inspect expanded `CEEENTRY`, `CEETERM`, and `CEEPPA` records, per-function DSA sizes, AMODE, USING/DROP ownership, literal pools, and all address displacements. R0 is a value register, never an address base. Automatic-object addresses are materialized through nonzero base registers; independently addressed globals are at least halfword aligned for LARL. See [IBM CEEENTRY](https://www.ibm.com/docs/en/zos/2.5.0?topic=macros-ceeentry-macro-generate-language-environment-conforming-prolog), [address-base rules](https://www.ibm.com/docs/en/hla-and-tf/1.6.0?topic=instruction-base-registers-absolute-addresses), and [LARL alignment](https://www.ibm.com/support/pages/apar/PH34824).

HLASM records use columns 1–71, with continuation in column 72 and resumed operands in column 16. Require no truncation, implicit-length, alignment, or conflicting-addressability diagnostics. Confirm that long branches reach their targets and that macro-generated literals remain addressable.

Writable C objects are initialized explicitly, including zero-filled storage, and the complete load module is bound NORENT. Do not change the binder option to RENT without implementing writable-static-area handling.

## Interoperability and execution

Calls implement the word-oriented non-XPLINK C argument image addressed by R1, not an OS list of argument pointers. Integer/pointer results use R15; floating-point and supported aggregate results use hidden destination storage. Compare both call directions against IBM C reference objects built with compatible options. The linkage reference is the [Language Environment Vendor Interfaces manual](https://publibz.boulder.ibm.com/epubs/pdf/cee1v201.pdf).

Run at least:

- Local strings, symbolic global pointers, parenthesized array indexing, signed shifts with unsigned counts, and floating logical conditions.
- Binary32/binary64 memory, conversions, exact constants, signed zeros, infinities, NaNs, and all ordered/unordered comparisons.
- Mixed variadic arguments and copies of `va_list`, small aggregate arguments/results, external runtime calls, and function pointers.
- Frames larger than 4 KiB, long branch spans, adjacent odd-sized globals, and repeated calls.
- Separate translation units with initialized/zero globals, public functions, and identically named private helpers.

For multiple translation units, assemble every source into GOFF, compile the runtime companion from the unit containing `main`, and include every object in the binder input. Only that unit supplies the synthetic `_main` process entry. Library-only units require a compatible LE/BFP caller.

For startup argument tests, edit the RUN step's `PARM`. The generated entry creates an empty `argv[0]`, splits space-separated arguments, preserves spaces inside double quotes, and writes `argv[argc] = NULL`. Test empty input and configured argument-buffer limits.

No native z/OS assembly, binder run, or execution has been performed in this workspace. These steps remain the acceptance gate, even when every local generation test passes.
