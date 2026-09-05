#include "cc.h"

CompilerContext compiler;

int main(int argc, char **argv)
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
	if (argc > 1)
	{
		IrFunction *function = &module.functions[0];
		IrBasicBlock *block = &function->blocks[0];
		mcc.nPreFile = -1;
		if (strcmp(argv[1], "operand") == 0)
		{
			block->instructions[1].left = function->nextValue;
		}
		else if (strcmp(argv[1], "type") == 0)
		{
			block->instructions[0].type.bits = 64U;
		}
		else if (strcmp(argv[1], "terminator") == 0)
		{
			block->instructions[0].opcode = IR_OP_RETURN;
			block->instructions[0].result = IR_VALUE_NONE;
		}
		else if (strcmp(argv[1], "storage") == 0)
		{
			module.functionCount = module.functionCapacity + 1;
		}
		else if (strcmp(argv[1], "relocation") == 0)
		{
			IrGlobal *global = irAddGlobal(&module, 2, i32, 4U, 4, 0, 0);
			unsigned int bytes = 0;
			irSetGlobalInitializer(global, &bytes, sizeof(bytes));
			irAddGlobalRelocation(global, 0U, 1, 0);
			global->relocations[0].offset = 3U;
		}
		else
		{
			return 2;
		}
		irVerifyModule(&module);
		return 3;
	}
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
