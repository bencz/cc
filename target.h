/* Compilation target descriptors and target-specific services. */

#ifndef CC_TARGET_H
#define CC_TARGET_H

typedef enum
{
	EXEC_CHARSET_IBM1047 = 0,
	EXEC_CHARSET_IBM037
} ExecutionCharset;

typedef struct _CompilerContext CompilerContext;

typedef struct _TargetDataLayout
{
	unsigned char pointerSize;
	unsigned char pointerAlignment;
	unsigned char shortSize;
	unsigned char intSize;
	unsigned char longSize;
	unsigned char floatSize;
	unsigned char doubleSize;
	unsigned char littleEndian;
	unsigned char subsequentMemberAlignmentLimit;
	unsigned char plainCharUnsigned;
} TargetDataLayout;

typedef struct _TargetDescriptor
{
	const char *name;
	const char *defaultExecutableExtension;
	const char *defaultSharedExtension;
	TargetDataLayout dataLayout;
	int supportsSharedOutput;
	int supportsAssemblyOutput;
	int supportsJclOutput;
	int requiresSingleTranslationUnit;
	void (*definePredefinedMacros)(CompilerContext *context);
	void (*addRuntimePrelude)(CompilerContext *context);
	void (*addStartup)(CompilerContext *context);
	unsigned char (*encodeExecutionByte)(CompilerContext *context, unsigned char value);
	void (*emitOutput)(CompilerContext *context, const char *outputFile);
} TargetDescriptor;

const TargetDescriptor *targetDefault(void);
const TargetDescriptor *targetFind(const char *name);
void targetValidate(const TargetDescriptor *target);

extern const TargetDescriptor targetX86Pe;
extern const TargetDescriptor targetZosHlasm;
extern const TargetDescriptor targetPpcLinux;
extern const TargetDescriptor targetPpcAix;

#endif /* CC_TARGET_H */
