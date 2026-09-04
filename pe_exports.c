/* Bounds-checked PE export and module-definition readers. */

#include "cc.h"

static unsigned int readU16(const unsigned char *data, size_t offset)
{
	return (unsigned int)data[offset] | ((unsigned int)data[offset + 1U] << 8);
}

static unsigned int readU32(const unsigned char *data, size_t offset)
{
	return (unsigned int)data[offset] | ((unsigned int)data[offset + 1U] << 8) |
	       ((unsigned int)data[offset + 2U] << 16) | ((unsigned int)data[offset + 3U] << 24);
}

static void requireRange(size_t fileSize, size_t offset, size_t length, const char *path)
{
	if (offset > fileSize || length > fileSize - offset)
	{
		error("PE reader", "'%s' contains an out-of-range PE field", path);
	}
}

static int equalIgnoreCase(const char *left, const char *right)
{
	while (*left != '\0' && *right != '\0')
	{
		if (tolower((unsigned char)*left) != tolower((unsigned char)*right))
		{
			return 0;
		}
		++left;
		++right;
	}
	return *left == *right;
}

static int hasSuffix(const char *text, const char *suffix)
{
	size_t textLength = strlen(text);
	size_t suffixLength = strlen(suffix);
	return textLength >= suffixLength && equalIgnoreCase(text + textLength - suffixLength, suffix);
}

static void copyText(char *destination, size_t capacity, const char *source, const char *path)
{
	size_t length = strlen(source);
	if (length >= capacity)
	{
		error("PE reader", "module name in '%s' is too long", path);
	}
	memcpy(destination, source, length + 1U);
}

static void registerExport(const char *name, int libraryIndex)
{
	const char *publicName = strncmp(name, "__p_", 4) == 0 ? name + 4 : name;
	int slot = hashPut((char *)publicName, NULL, &cd.hash);
	Name *definedVariable = getNameFromTable(globTable, NM_VAR, slot);
	intptr_t attributes = (intptr_t)cd.hash.tbl[slot].val;
	if (definedVariable != NULL && definedVariable->addrType != AD_IMPORT)
	{
		return;
	}
	if ((attributes & AT_USER) != 0)
	{
		return;
	}
	if ((attributes & AT_IMPT) != 0 && (attributes & AT_ADDR) != libraryIndex)
	{
		cd.hash.tbl[slot].val = (void *)(intptr_t)(AT_IMPT | AT_AMBIGUOUS_IMPORT);
		return;
	}
	cd.hash.tbl[slot].val = (void *)(intptr_t)(AT_IMPT | libraryIndex);
}

static size_t rvaToOffset(const unsigned char *data,
	                      size_t fileSize,
	                      size_t optionalOffset,
	                      unsigned int sectionCount,
	                      unsigned int optionalSize,
	                      unsigned int rva,
	                      const char *path)
{
	unsigned int sectionIndex;
	unsigned int headerSize;
	size_t sectionOffset;
	requireRange(fileSize, optionalOffset + 60U, 4U, path);
	headerSize = readU32(data, optionalOffset + 60U);
	if (rva < headerSize)
	{
		requireRange(fileSize, rva, 1U, path);
		return rva;
	}
	sectionOffset = optionalOffset + optionalSize;
	requireRange(fileSize, sectionOffset, (size_t)sectionCount * 40U, path);
	for (sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex)
	{
		size_t current = sectionOffset + (size_t)sectionIndex * 40U;
		unsigned int virtualSize = readU32(data, current + 8U);
		unsigned int virtualAddress = readU32(data, current + 12U);
		unsigned int rawSize = readU32(data, current + 16U);
		unsigned int rawOffset = readU32(data, current + 20U);
		unsigned int mappedSize = virtualSize > rawSize ? virtualSize : rawSize;
		if (rva >= virtualAddress && rva - virtualAddress < mappedSize)
		{
			size_t result = (size_t)rawOffset + (rva - virtualAddress);
			requireRange(fileSize, result, 1U, path);
			return result;
		}
	}
	error("PE reader", "'%s' contains an unmapped export RVA", path);
}

static char *checkedString(unsigned char *data, size_t fileSize, size_t offset, const char *path)
{
	size_t cursor = offset;
	requireRange(fileSize, offset, 1U, path);
	while (cursor < fileSize && data[cursor] != '\0')
	{
		++cursor;
	}
	if (cursor == fileSize)
	{
		error("PE reader", "'%s' contains an unterminated export name", path);
	}
	return (char *)(data + offset);
}

static void loadPeExports(const char *path,
	                      const char *requestedName,
	                      int libraryIndex,
	                      char *moduleName,
	                      size_t moduleNameCapacity)
{
	FILE *file = fopen((char *)path, "rb");
	long length;
	unsigned char *data;
	size_t fileSize;
	size_t peOffset;
	size_t optionalOffset;
	unsigned int sectionCount;
	unsigned int optionalSize;
	unsigned int magic;
	size_t directoryOffset;
	unsigned int exportRva;
	size_t exportOffset;
	unsigned int nameCount;
	unsigned int namesRva;
	size_t namesOffset;
	unsigned int index;
	if (file == NULL)
	{
		error("PE reader", "cannot open import library '%s'", path);
	}
	if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 ||
	    fseek(file, 0, SEEK_SET) != 0)
	{
		fclose(file);
		error("PE reader", "cannot measure import library '%s'", path);
	}
	fileSize = (size_t)length;
	data = xalloc(fileSize);
	if (fread(data, 1U, fileSize, file) != fileSize)
	{
		fclose(file);
		free(data);
		error("PE reader", "cannot read import library '%s'", path);
	}
	fclose(file);
	requireRange(fileSize, 0, 64U, path);
	if (readU16(data, 0) != 0x5A4DU)
	{
		free(data);
		error("PE reader", "'%s' is not a PE image", path);
	}
	peOffset = readU32(data, 0x3CU);
	requireRange(fileSize, peOffset, 24U, path);
	if (readU32(data, peOffset) != 0x00004550U)
	{
		free(data);
		error("PE reader", "'%s' has an invalid PE signature", path);
	}
	sectionCount = readU16(data, peOffset + 6U);
	optionalSize = readU16(data, peOffset + 20U);
	optionalOffset = peOffset + 24U;
	requireRange(fileSize, optionalOffset, optionalSize, path);
	magic = readU16(data, optionalOffset);
	if (magic == 0x10BU)
	{
		directoryOffset = optionalOffset + 96U;
	}
	else if (magic == 0x20BU)
	{
		directoryOffset = optionalOffset + 112U;
	}
	else
	{
		free(data);
		error("PE reader", "'%s' has an unsupported optional-header format", path);
	}
	requireRange(fileSize, directoryOffset, 8U, path);
	exportRva = readU32(data, directoryOffset);
	if (exportRva == 0U)
	{
		free(data);
		error("PE reader", "'%s' has no export directory", path);
	}
	exportOffset = rvaToOffset(
	    data, fileSize, optionalOffset, sectionCount, optionalSize, exportRva, path);
	requireRange(fileSize, exportOffset, 40U, path);
	nameCount = readU32(data, exportOffset + 24U);
	namesRva = readU32(data, exportOffset + 32U);
	namesOffset = rvaToOffset(
	    data, fileSize, optionalOffset, sectionCount, optionalSize, namesRva, path);
	requireRange(fileSize, namesOffset, (size_t)nameCount * 4U, path);
	if (readU32(data, exportOffset + 12U) != 0U)
	{
		size_t moduleOffset = rvaToOffset(data,
		                                      fileSize,
		                                      optionalOffset,
		                                      sectionCount,
		                                      optionalSize,
		                                      readU32(data, exportOffset + 12U),
		                                      path);
		copyText(moduleName,
		         moduleNameCapacity,
		         checkedString(data, fileSize, moduleOffset, path),
		         path);
	}
	else
	{
		copyText(moduleName, moduleNameCapacity, requestedName, path);
	}
	for (index = 0; index < nameCount; ++index)
	{
		unsigned int nameRva = readU32(data, namesOffset + (size_t)index * 4U);
		size_t nameOffset = rvaToOffset(
		    data, fileSize, optionalOffset, sectionCount, optionalSize, nameRva, path);
		registerExport(checkedString(data, fileSize, nameOffset, path), libraryIndex);
	}
	free(data);
}

static char *skipSpace(char *text)
{
	while (*text == ' ' || *text == '\t')
	{
		++text;
	}
	return text;
}

static void loadDefExports(const char *path,
	                       const char *requestedName,
	                       int libraryIndex,
	                       char *moduleName,
	                       size_t moduleNameCapacity)
{
	FILE *file = fopen((char *)path, "r");
	char line[4096];
	int inExports = 0;
	copyText(moduleName, moduleNameCapacity, requestedName, path);
	if (file == NULL)
	{
		error("DEF reader", "cannot open module-definition file '%s'", path);
	}
	while (fgets(line, sizeof(line), file) != NULL)
	{
		char *comment = strchr(line, ';');
		char *cursor;
		char *end;
		if (comment != NULL)
		{
			*comment = '\0';
		}
		cursor = skipSpace(line);
		if (*cursor == '\0' || *cursor == '\r' || *cursor == '\n')
		{
			continue;
		}
		end = cursor;
		while (*end != '\0' && *end != ' ' && *end != '\t' && *end != '\r' && *end != '\n')
		{
			++end;
		}
		if (*end != '\0')
		{
			*end++ = '\0';
		}
		if (equalIgnoreCase(cursor, "LIBRARY"))
		{
			char *name = skipSpace(end);
			char quote = *name == '\'' || *name == '"' ? *name++ : '\0';
			char *nameEnd = name;
			while (*nameEnd != '\0' && *nameEnd != '\r' && *nameEnd != '\n' &&
			       (quote != '\0' ? *nameEnd != quote : *nameEnd != ' ' && *nameEnd != '\t'))
			{
				++nameEnd;
			}
			*nameEnd = '\0';
			if (*name != '\0')
			{
				copyText(moduleName, moduleNameCapacity, name, path);
			}
			continue;
		}
		if (equalIgnoreCase(cursor, "EXPORTS"))
		{
			inExports = 1;
			continue;
		}
		if (!inExports || equalIgnoreCase(cursor, "DESCRIPTION") ||
		    equalIgnoreCase(cursor, "SECTIONS") || equalIgnoreCase(cursor, "IMPORTS"))
		{
			continue;
		}
		end = cursor;
		while (*end != '\0' && *end != '=' && *end != '@' && *end != ' ' && *end != '\t')
		{
			++end;
		}
		*end = '\0';
		if (*cursor != '\0')
		{
			registerExport(cursor, libraryIndex);
		}
	}
	if (fclose(file) != 0)
	{
		error("DEF reader", "cannot close module-definition file '%s'", path);
	}
}

void peLoadExportSymbols(const char *path,
	                     const char *requestedName,
	                     int libraryIndex,
	                     char *moduleName,
	                     size_t moduleNameCapacity)
{
	if (hasSuffix(path, ".def"))
	{
		loadDefExports(path, requestedName, libraryIndex, moduleName, moduleNameCapacity);
	}
	else
	{
		loadPeExports(path, requestedName, libraryIndex, moduleName, moduleNameCapacity);
	}
}
