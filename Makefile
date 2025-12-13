# Makefile para o compilador CC
#
# Uso:
#   make              - Compila o compilador (detecta plataforma automaticamente)
#   make bootstrap    - Auto-compilação usando o próprio compilador
#   make clean        - Remove arquivos objeto e executável
#   make test-x86     - Compila todos os testes para x86/PE (Windows only)
#   make test-hlasm   - Compila todos os testes para HLASM
#
# Backends disponíveis (escolha em runtime):
#   - x86/PE: gera executáveis Windows (apenas em Windows)
#   - HLASM: gera código assembler para mainframe z/OS (cross-platform, use -hlasm)

# Detecta o sistema operacional
ifeq ($(OS),Windows_NT)
    PLATFORM = windows
    TARGET = cc.exe
    RM = del /Q
    RMDIR = rmdir /S /Q
    MKDIR = if not exist build mkdir build
    LDFLAGS = -lkernel32
    PATHSEP = \\
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        PLATFORM = macos
    else
        PLATFORM = linux
    endif
    TARGET = cc
    RM = rm -f
    RMDIR = rm -rf
    MKDIR = mkdir -p build
    LDFLAGS =
    PATHSEP = /
endif

# Diretórios
BUILD_DIR = build
TEST_DIR = tests
TEST_OUT_X86 = $(BUILD_DIR)/tests/x86
TEST_OUT_HLASM = $(BUILD_DIR)/tests/hlasm

CC = gcc
CFLAGS = -Wall -O2

# Arquivos fonte
SRCS = main.c util.c prepro.c lexer.c codegen.c parser.c expr.c \
       cc_platform.c backend_x86.c backend_hlasm.c

# Objetos na pasta build/
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# Arquivos de teste
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_NAMES = $(notdir $(basename $(TEST_SRCS)))

# =============================================================================
# Regras principais
# =============================================================================

all: $(BUILD_DIR) $(TARGET)
	@echo "Build complete for $(PLATFORM)"
	@echo "Use -hlasm flag for HLASM output (cross-platform)"
	@echo "Without -hlasm, generates x86/PE output (Windows only)"

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Criar diretório build
$(BUILD_DIR):
	$(MKDIR)

# Regra para compilar arquivos .c em build/*.o
$(BUILD_DIR)/%.o: %.c cc.h cc_platform.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# =============================================================================
# Testes
# =============================================================================

# Criar diretórios de saída de testes
$(TEST_OUT_X86):
	@mkdir -p $(TEST_OUT_X86)

$(TEST_OUT_HLASM):
	@mkdir -p $(TEST_OUT_HLASM)

# Compilar todos os testes para x86/PE (Windows only)
test-x86: $(TARGET) $(TEST_OUT_X86)
	@echo "=== Compilando testes para x86/PE ==="
ifeq ($(PLATFORM),windows)
	@for %%f in ($(TEST_DIR)\*.c) do ( \
		echo Compilando %%~nf.c ... && \
		./$(TARGET) %%f -o $(TEST_OUT_X86)/%%~nf.exe \
	)
	@echo "=== Testes x86 gerados em $(TEST_OUT_X86)/ ==="
else
	@echo "Erro: Backend x86/PE só disponível no Windows"
	@echo "Use 'make test-hlasm' para gerar código HLASM"
endif

# Compilar todos os testes para HLASM
test-hlasm: $(TARGET) $(TEST_OUT_HLASM)
	@echo "=== Compilando testes para HLASM ==="
	@for src in $(TEST_SRCS); do \
		name=$$(basename $$src .c); \
		echo "Compilando $$name.c ..."; \
		./$(TARGET) $$src -hlasm -o $(TEST_OUT_HLASM)/$$name.asm; \
	done
	@echo "=== Testes HLASM gerados em $(TEST_OUT_HLASM)/ ==="

# Compilar um teste específico
test-%: $(TARGET)
	@echo "Compilando teste: $*"
ifeq ($(PLATFORM),windows)
	@mkdir -p $(TEST_OUT_X86)
	./$(TARGET) $(TEST_DIR)/$*.c -asm -o $(TEST_OUT_X86)/$*.asm
else
	@mkdir -p $(TEST_OUT_HLASM)
	./$(TARGET) $(TEST_DIR)/$*.c -hlasm -o $(TEST_OUT_HLASM)/$*.asm
endif

# =============================================================================
# Auto-compilação (bootstrap)
# =============================================================================

# Macros de plataforma para auto-compilação
ifeq ($(PLATFORM),windows)
    PLATFORM_DEFINES = -D_WIN32
    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
        PLATFORM_DEFINES += -D_WIN64
    endif
else ifeq ($(PLATFORM),macos)
    PLATFORM_DEFINES = -D__APPLE__ -D__MACH__ -DPLATFORM_UNIX
else
    PLATFORM_DEFINES = -D__linux__ -DPLATFORM_UNIX
endif

# Auto-compilação usando o próprio compilador
bootstrap: $(TARGET)
	@echo "=== Auto-compilação (bootstrap) ==="
	@echo "Plataforma detectada: $(PLATFORM)"
	@echo "Defines: $(PLATFORM_DEFINES)"
	./$(TARGET) -hlasm $(PLATFORM_DEFINES) $(SRCS) -o $(BUILD_DIR)/cc_bootstrap.asm
	@echo "=== Bootstrap gerado em $(BUILD_DIR)/cc_bootstrap.asm ==="

# =============================================================================
# Limpeza
# =============================================================================

clean:
	-$(RM) $(OBJS) $(TARGET)
ifeq ($(OS),Windows_NT)
	-$(RMDIR) $(BUILD_DIR) 2>nul
else
	-$(RMDIR) $(BUILD_DIR)
endif

# Limpar apenas testes
clean-tests:
	-$(RM) $(TEST_OUT_DIR)/*

# =============================================================================
# Informações
# =============================================================================

info:
	@echo "Plataforma: $(PLATFORM)"
	@echo "Compilador: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Target: $(TARGET)"
	@echo "Diretório build: $(BUILD_DIR)"
	@echo "Diretório testes: $(TEST_DIR)"
	@echo ""
	@echo "Testes disponíveis:"
	@for src in $(TEST_SRCS); do echo "  - $$(basename $$src)"; done

.PHONY: all clean clean-tests test-x86 test-hlasm info bootstrap
