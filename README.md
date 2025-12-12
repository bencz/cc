# CC - Multi-Platform C Compiler

A simple C compiler with support for multiple backends:
- **x86/PE** - Generates Windows executables (EXE/DLL)
- **HLASM** - Generates assembler code for IBM z/OS mainframes

## Supported Platforms

| Platform | x86/PE Backend | HLASM Backend |
|----------|----------------|---------------|
| Windows  | Full           | Available     |
| Linux    | No             | Available     |
| macOS    | No             | Available     |

## Project Structure

```
cc/
├── cc.h              # Main header with types and declarations
├── cc_platform.h     # Cross-platform abstractions
├── cc_platform.c     # Cross-platform implementation
├── main.c            # Entry point and argument processing
├── util.c            # Utility functions (hash, alloc, error)
├── prepro.c          # Preprocessor (#include, #define, #pragma)
├── lexer.c           # Lexical analysis (tokenization)
├── parser.c          # Parser, declarations and statements
├── expr.c            # Expression processing
├── codegen.c         # Intermediate code generation interface
├── backend_x86.c     # x86/PE backend for Windows
├── backend_hlasm.c   # HLASM backend for mainframe
├── include/          # Standard header files
├── tests/            # Test cases
└── Makefile          # Multi-platform build script
```

## Modules

### cc.h
Main header containing all type definitions, structures, constants and function declarations shared between modules.

### main.c
Compiler entry point. Processes command line arguments and coordinates compilation phases.

### util.c
Utility functions:
- Error handling (`error`)
- Memory allocation (`xalloc`, `xrealloc`, `xstrdup`)
- String manipulation (`xstrcpy`, `xstrlen`)
- Hash tables (`initHash`, `put`, `get`, `freeHash`)

### prepro.c
Preprocessor:
- `#include` processing
- `#define` processing
- `#pragma` processing
- Startup code generation

### lexer.c
Lexical analysis:
- Source code tokenization
- Symbol, number, string identification

### parser.c
Parser and syntactic analysis:
- Variable and function declarations
- Structures and enums
- Statements (if, for, while, switch, etc.)
- Symbol table

### expr.c
Expression processing:
- Arithmetic expressions
- Logical and relational expressions
- Assignments
- Function calls

### codegen.c
Code generation interface:
- Intermediate instruction emission
- Control flow (jumps, labels)
- Data emission

### backend_x86.c
x86/Windows specific backend:
- x86 instruction table
- Machine code generation
- PE linker (EXE/DLL)
- DLL import/export

### backend_hlasm.c
HLASM backend (mainframe):
- HLASM code generation
- z/Architecture instruction mapping
- z/OS calling conventions

## Building

### Standard Build
```bash
make              # Build without HLASM support
make hlasm        # Build with HLASM backend support
make clean        # Remove object files
```

### Windows (MinGW/MSYS2)
```bash
make              # Generates cc.exe with x86/PE backend
make hlasm        # Adds HLASM support
```

### Linux/macOS
```bash
make hlasm        # Generates cc with HLASM backend (required)
```

### Testing
```bash
make test-hlasm   # Compile all tests to HLASM
```

## Usage

```bash
cc [options] file...

Options:
  -o <file>  Set output file name
  -hlasm     Generate HLASM code for mainframe
  -shared    Generate DLL file (Windows only)
  -l<name>   Link with <name>.dll (Windows only)
  -E         Show expression tree
  -token     List tokens
  -asm       Generate intermediate code listing
  --lines    Line count
```

### Examples

```bash
# Windows: Compile program to PE executable
cc hello.c -o hello.exe

# Windows: Compile with additional DLL
cc program.c -lcomctl32 -o program.exe

# Windows: Generate DLL
cc -shared mylib.c -o mylib.dll

# Any platform: Generate HLASM code
cc hello.c -hlasm -o hello.asm

# View intermediate code
cc -asm program.c
```

## HLASM Backend

The HLASM backend generates assembler code for IBM z/OS mainframes. To use:

```bash
# Build the compiler with HLASM support
make hlasm

# Generate HLASM code
./cc program.c -hlasm -o program.asm
```

### Register Mapping

#### General Purpose Registers (GPR)

| x86 Virtual | z/Architecture | Usage                    |
|-------------|----------------|--------------------------|
| EAX         | R2             | Accumulator, return value|
| ECX         | R3             | Counter                  |
| EDX         | R4             | Data                     |
| EBP         | R11            | Frame pointer            |
| ESP         | R13            | Save area pointer        |

#### Floating Point Registers (FPR)

| Register | Usage                              |
|----------|------------------------------------|
| F0       | Primary FPU operations, return     |
| F2       | Secondary operand                  |
| F4, F6   | Additional work registers          |
| F8-F15   | Callee-saved                       |

### z/OS Calling Conventions

**Parameter Passing:**
- Parameters passed via list pointed by R1
- Each entry is a pointer to the argument
- Last pointer has VL bit set (high-order bit = 1)

**Return Values:**
- Integer return in R15
- Float/double return in F0

**Save Area (72 bytes):**
```
Offset  Content
+0      Reserved
+4      Pointer to previous save area
+8      Pointer to next save area
+12     R14 (return address)
+16     R15 (entry point)
+20     R0-R12 (13 registers × 4 bytes)
```

**Register Preservation:**
- R0-R4: Not preserved (work registers)
- R5-R10: Preserved by callee
- R11-R14: Preserved by callee
- R15: Not preserved (return code)

### Floating Point Support

The backend supports IEEE floating point operations using z/Architecture BFP instructions:

| Operation        | HLASM Instruction |
|------------------|-------------------|
| Load double      | LD                |
| Store double     | STD               |
| Add              | AD / ADR          |
| Subtract         | SD / SDR          |
| Multiply         | MD / MDR          |
| Divide           | DD / DDR          |
| Compare          | CD / CDR          |
| Negate           | LCDR              |
| Int → Double     | CDFBR             |
| Double → Int     | CFDBR             |

### String Encoding

The backend includes automatic ASCII to EBCDIC conversion for string literals, as z/OS uses EBCDIC as its native character encoding.

## Architecture

```
+-------------+
|   C Code    |
+------+------+
       |
       v
+-------------+
|   Prepro    |  #include, #define, #pragma
+------+------+
       |
       v
+-------------+
|   Lexer     |  Tokenization
+------+------+
       |
       v
+-------------+
|   Parser    |  Syntactic analysis
+------+------+
       |
       v
+-------------+
|  Codegen    |  Intermediate code
+------+------+
       |
       v
+-------------+     +-------------+
| Backend x86 | OR  |Backend HLASM|
+------+------+     +------+------+
       |                   |
       v                   v
+-------------+     +-------------+
|  EXE/DLL    |     |    .asm     |
+-------------+     +-------------+
```

## Limitations

- Supports a subset of C (not full C standard)
- x86/PE backend only for Windows 32-bit
- HLASM backend generates functional code but may need adjustments for specific cases

## License

This project is licensed under the **Unlicense** - a public domain dedication.

This means you can copy, modify, distribute and perform the work, even for commercial purposes, all without asking permission.

See the [LICENSE](LICENSE) file for details, or visit: https://unlicense.org/
