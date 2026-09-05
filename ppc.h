/* PowerPC32 linkage policy, shared by parameter and call lowering. */
#ifndef CC_PPC_H
#define CC_PPC_H

#include "ir.h"

typedef struct _PpcArgumentCursor
{
	int words;
	int floating;
	int stack;
} PpcArgumentCursor;

typedef struct _PpcArgumentLocation
{
	int generalRegister;
	int floatingRegister;
	int stackOffset;
	int words;
} PpcArgumentLocation;

typedef struct _PpcAbi
{
	const char *name;
	int linkageSize;
	int minimumArgumentSize;
	int savedLinkOffset;
	int floatingRegisterCount;
	int hasDescriptors;
	void (*classify)(PpcArgumentCursor *, IrType, PpcArgumentLocation *);
	void (*textSection)(FILE *);
	void (*functionEntry)(FILE *, const char *, int);
	void (*functionEnd)(FILE *, const char *);
	void (*symbolAddress)(FILE *, int, const char *, unsigned int);
	void (*directCall)(FILE *, const char *);
	void (*indirectCall)(FILE *);
} PpcAbi;

extern const PpcAbi ppcSysvAbi;
extern const PpcAbi ppcAixAbi;
void ppcEmitModule(const char *path, const PpcAbi *abi);

#endif
