/* Target registry. */

#include "cc.h"

const TargetDescriptor *targetDefault(void)
{
	return &targetX86Pe;
}

const TargetDescriptor *targetFind(const char *name)
{
	if (strcmp(name, targetX86Pe.name) == 0)
	{
		return &targetX86Pe;
	}
	if (strcmp(name, targetZosHlasm.name) == 0)
	{
		return &targetZosHlasm;
	}
	return NULL;
}

void targetValidate(const TargetDescriptor *target)
{
	if (target == NULL)
	{
		error("target", "target descriptor is null");
	}
	if (target->name == NULL)
	{
		error("target", "target name is missing");
	}
	if (target->definePredefinedMacros == NULL)
	{
		error("target", "target '%s' has no predefined-macro callback", target->name);
	}
	if (target->addRuntimePrelude == NULL)
	{
		error("target", "target '%s' has no runtime-prelude callback", target->name);
	}
	if (target->addStartup == NULL)
	{
		error("target", "target '%s' has no startup callback", target->name);
	}
	if (target->encodeExecutionByte == NULL)
	{
		error("target", "target '%s' has no execution-byte callback", target->name);
	}
	if (target->emitOutput == NULL)
	{
		error("target", "target '%s' has no output callback", target->name);
	}
	if (target->dataLayout.pointerSize == 0 || target->dataLayout.pointerAlignment == 0 ||
	    target->dataLayout.shortSize == 0 || target->dataLayout.intSize == 0 ||
	    target->dataLayout.longSize == 0 || target->dataLayout.floatSize == 0 ||
	    target->dataLayout.doubleSize == 0)
	{
		error("target", "target '%s' has an invalid data layout", target->name);
	}
}
