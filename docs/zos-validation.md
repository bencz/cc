# Validation on z/OS 2.5

1. Transfer the generated `.jcl` as IBM-1047 text. If the installation uses IBM-037 as its execution character set, generate literals with `--exec-charset=ibm-037`.
2. Adapt only the JOB card to the site's conventions. The job invokes `ASMA90` and `IEWL` with `CEE.SCEEMAC` and `CEE.SCEELKED`.
3. Submit the job and require return code zero from the `ASM`, `BIND`, and `RUN` steps. The generated `COND` clauses prevent a later step from hiding a nonzero return code from an earlier one.
4. Inspect the HLASM listing for `ASMA` messages, truncation, implicit lengths, `USING` conflicts, alignment errors, and `RENT` violations.
5. Inspect the binder map for the generated `Fxxxxxx` entry point, AMODE 31, RMODE ANY, and complete resolution of external aliases against the Language Environment libraries.
6. For argument-handling tests, edit `PARM=''` in the `RUN` step. The entry code creates an empty `argv[0]`, splits the remaining arguments on spaces, preserves spaces inside double quotes, and terminates the vector with `argv[argc] == NULL`.

For a program with multiple translation units, generate one `.asm` file per `.c` file, assemble each source into a GOFF object, and concatenate the corresponding object DD statements before the binder step's `NAME` statement. Only the translation unit that defines `main` may provide the synthetic `_main` entry.
