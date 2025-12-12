# Makefile para o compilador CC
#
# Uso:
#   make              - Compila o compilador (detecta plataforma automaticamente)
#   make hlasm        - Compila com suporte ao backend HLASM
#   make clean        - Remove arquivos objeto e executável
#   make test         - Compila todos os testes
#   make test-hlasm   - Compila todos os testes para HLASM
#
# Plataformas suportadas:
#   - Windows (gera executáveis PE)
#   - Linux/macOS (apenas backend HLASM)

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
TEST_OUT_DIR = $(BUILD_DIR)/tests

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
	@echo "Use -hlasm flag for HLASM output on non-Windows platforms"

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Build com suporte HLASM habilitado
hlasm: CFLAGS += -DUSE_HLASM_BACKEND
hlasm: $(BUILD_DIR) $(TARGET)
	@echo "Build complete with HLASM backend support"

# Criar diretório build
$(BUILD_DIR):
	$(MKDIR)

# Regra para compilar arquivos .c em build/*.o
$(BUILD_DIR)/%.o: %.c cc.h cc_platform.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# =============================================================================
# Testes
# =============================================================================

# Criar diretório de saída de testes
$(TEST_OUT_DIR):
	@mkdir -p $(TEST_OUT_DIR)

# Compilar todos os testes (Windows - gera .exe)
test: $(TARGET) $(TEST_OUT_DIR)
	@echo "=== Compilando testes ==="
ifeq ($(PLATFORM),windows)
	@for %%f in ($(TEST_DIR)\*.c) do ( \
		echo Compilando %%~nf.c ... && \
		./$(TARGET) %%f -o $(TEST_OUT_DIR)/%%~nf.exe \
	)
else
	@echo "Use 'make test-hlasm' em plataformas não-Windows"
endif
	@echo "=== Testes compilados em $(TEST_OUT_DIR)/ ==="

# Compilar todos os testes para HLASM
test-hlasm: hlasm $(TEST_OUT_DIR)
	@echo "=== Compilando testes para HLASM ==="
	@for src in $(TEST_SRCS); do \
		name=$$(basename $$src .c); \
		echo "Compilando $$name.c ..."; \
		./$(TARGET) $$src -hlasm -o $(TEST_OUT_DIR)/$$name.asm; \
	done
	@echo "=== Testes HLASM gerados em $(TEST_OUT_DIR)/ ==="

# Compilar um teste específico
test-%: $(TARGET)
	@echo "Compilando teste: $*"
ifeq ($(PLATFORM),windows)
	./$(TARGET) $(TEST_DIR)/$*.c -o $(BUILD_DIR)/$*.exe
else
	./$(TARGET) $(TEST_DIR)/$*.c -hlasm -o $(BUILD_DIR)/$*.asm
endif

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

.PHONY: all clean clean-tests test test-hlasm hlasm info
