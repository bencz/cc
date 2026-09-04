#include "cc.h"

CompilerContext compiler;

int main(void)
{
	IrModule module;
	IrBuilder builder;
	IrType i32 = irTypeInteger(32U, 0, 4U);
	IrValueId constant;
	FILE *output;

	irModuleInit(&module);
	irBuilderInit(&builder, &module);
	(void)irBuilderBeginFunction(&builder, 1, i32, NULL, 0);
	constant = irBuilderEmitInteger(&builder, i32, 42U);
	irBuilderEmitReturn(&builder, constant);
	irBuilderEndFunction(&builder);
	irVerifyModule(&module);

	output = tmpfile();
	if (output == NULL)
	{
		irModuleFree(&module);
		return 1;
	}
	irDumpModule(&module, output);
	if (ftell(output) <= 0)
	{
		fclose(output);
		irModuleFree(&module);
		return 2;
	}
	fclose(output);
	irModuleFree(&module);
	return 0;
}
