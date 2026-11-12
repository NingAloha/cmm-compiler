# Toolchain

CC := gcc
FLEX := flex
BISON := bison
XELATEX := xelatex
ZIP := zip
OPEN := open
CLANG_FORMAT := clang-format
SHFMT := shfmt

# Compilation

CPPFLAGS := -Isrc/frontend -MMD -MP
CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra

# Build paths

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
TARGET := $(BUILD_DIR)/parser
ROOT_TARGET := parser
COURSE_TARGET := cc

# Generated frontend sources (syntax.tab.c includes lex.yy.c)

PARSER_SPEC := src/frontend/syntax.y
LEXER_SPEC := src/frontend/lexer.l
PARSER_SOURCE := $(BUILD_DIR)/syntax.tab.c
LEXER_SOURCE := $(BUILD_DIR)/lex.yy.c

# Map module sources (src/x/y.c) to object files (build/obj/x/y.o)

obj_of = $(patsubst src/%.c,$(OBJ_DIR)/%.o,$(1))
test_obj_of = $(patsubst tests/%.c,$(OBJ_DIR)/tests/%.o,$(1))

# Hand-written sources, grouped by module

FRONTEND_SOURCES := src/frontend/tree.c
SEMANTIC_CORE_SOURCES := src/semantic/type.c src/semantic/symbol.c \
	src/semantic/util.c
SEMANTIC_SOURCES := src/semantic/semantic.c src/semantic/sem_decl.c \
	src/semantic/sem_stmt.c src/semantic/sem_expr.c \
	$(SEMANTIC_CORE_SOURCES)
IR_CORE_SOURCES := src/ir/ir.c
IR_SOURCES := $(IR_CORE_SOURCES) src/ir/generator.c src/ir/gen_array.c \
	src/ir/gen_expr.c src/ir/gen_stmt.c src/ir/gen_decl.c
DRIVER_SOURCES := src/driver/main.c

COMPILER_SOURCES := $(FRONTEND_SOURCES) $(SEMANTIC_SOURCES) $(IR_SOURCES) \
	$(DRIVER_SOURCES)
COMPILER_OBJECTS := $(OBJ_DIR)/syntax.tab.o $(call obj_of,$(COMPILER_SOURCES))

# Tests

TEST_STAGES := 01 02 03
TEST_TARGETS := $(addprefix test-,$(TEST_STAGES))
TEST_LIB := tests/lib.sh
TEST_RUNNER := tests/run_tests.sh
IR_TEST_RUNNER := tests/run_ir_tests.sh
IR_REJECTION_TEST_RUNNER := tests/run_ir_rejection_tests.sh
TEST_SCRIPTS := $(TEST_LIB) $(TEST_RUNNER) $(IR_TEST_RUNNER) \
	$(IR_REJECTION_TEST_RUNNER)

STAGE_01_MANIFEST := tests/stage-01/expected.tsv
STAGE_02_MANIFEST := tests/stage-02/semantic_expected.tsv
STAGE_03_MANIFEST := tests/stage-03/expected.tsv
STAGE_03_REJECTIONS := tests/stage-03/rejected.tsv

# Unit tests: each links its test source with the module sources it covers

UNIT_TEST_BIN_DIR := $(BUILD_DIR)/tests

TYPE_TEST := $(UNIT_TEST_BIN_DIR)/test_type
TYPE_TEST_OBJECTS := $(call test_obj_of,tests/stage-02/test_type.c) \
	$(call obj_of,src/semantic/type.c src/semantic/util.c)

SYMBOL_TEST := $(UNIT_TEST_BIN_DIR)/test_symbol
SYMBOL_TEST_OBJECTS := $(call test_obj_of,tests/stage-02/test_symbol.c) \
	$(call obj_of,$(SEMANTIC_CORE_SOURCES))

IR_TEST := $(UNIT_TEST_BIN_DIR)/test_ir
IR_TEST_OBJECTS := $(call test_obj_of,tests/stage-03/test_ir.c) \
	$(call obj_of,$(IR_CORE_SOURCES))

# Course submission

COURSEWORK_DIR := coursework
REPORT_SOURCE := $(COURSEWORK_DIR)/report.tex
REPORT_PDF := $(COURSEWORK_DIR)/report.pdf
PACKAGE_DIR := $(BUILD_DIR)/submission
SUBMIT_ZIP := $(COURSEWORK_DIR)/submit.zip

.PHONY: all clean pack format test report $(TEST_TARGETS) $(REPORT_PDF)

all: $(TARGET) $(ROOT_TARGET) $(COURSE_TARGET)

# Generated sources

$(BUILD_DIR) $(OBJ_DIR) $(UNIT_TEST_BIN_DIR):
	mkdir -p $@

$(PARSER_SOURCE): $(PARSER_SPEC) | $(BUILD_DIR)
	$(BISON) -d -v -o $@ $<

$(LEXER_SOURCE): $(LEXER_SPEC) $(PARSER_SOURCE) | $(BUILD_DIR)
	$(FLEX) -o $@ $<

# Objects

$(OBJ_DIR)/syntax.tab.o: $(PARSER_SOURCE) $(LEXER_SOURCE) | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/tests/%.o: tests/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Executables

$(TARGET): $(COMPILER_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(ROOT_TARGET): $(TARGET)
	ln -sf $(TARGET) $@

$(COURSE_TARGET): $(TARGET)
	ln -sf $(TARGET) $@

# Unit test binaries

$(TYPE_TEST): $(TYPE_TEST_OBJECTS) | $(UNIT_TEST_BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(SYMBOL_TEST): $(SYMBOL_TEST_OBJECTS) | $(UNIT_TEST_BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(IR_TEST): $(IR_TEST_OBJECTS) | $(UNIT_TEST_BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Test targets

test: $(TEST_TARGETS)

test-01: $(TARGET) $(TEST_SCRIPTS) $(STAGE_01_MANIFEST)
	@sh $(TEST_RUNNER) $(TARGET) $(STAGE_01_MANIFEST) --tree

test-02: $(TYPE_TEST) $(SYMBOL_TEST) $(TARGET) $(TEST_SCRIPTS) \
	$(STAGE_02_MANIFEST)
	@$(TYPE_TEST)
	@$(SYMBOL_TEST)
	@sh $(TEST_RUNNER) $(TARGET) $(STAGE_02_MANIFEST)

test-03: $(IR_TEST) $(TARGET) $(TEST_SCRIPTS) $(STAGE_03_MANIFEST) \
	$(STAGE_03_REJECTIONS)
	@$(IR_TEST)
	@sh $(IR_TEST_RUNNER) $(TARGET) $(STAGE_03_MANIFEST)
	@sh $(IR_REJECTION_TEST_RUNNER) $(TARGET) $(STAGE_03_REJECTIONS)

# Formatting

format:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { \
		echo "clang-format is not installed. Run: brew install clang-format" >&2; \
		exit 1; }
	@command -v $(SHFMT) >/dev/null 2>&1 || { \
		echo "shfmt is not installed. Run: brew install shfmt" >&2; \
		exit 1; }
	find src tests -type f \( -name '*.c' -o -name '*.h' \) -print0 | \
		xargs -0 $(CLANG_FORMAT) -i
	find tests -type f -name '*.sh' -print0 | xargs -0 $(SHFMT) -i 2 -w

# Course submission

$(REPORT_PDF): $(REPORT_SOURCE) | $(BUILD_DIR)
	$(XELATEX) -interaction=nonstopmode -output-directory=$(BUILD_DIR) $<
	$(XELATEX) -interaction=nonstopmode -output-directory=$(BUILD_DIR) $<
	cp $(BUILD_DIR)/report.pdf $@

report: $(REPORT_PDF)
	$(OPEN) $(REPORT_PDF)

pack: $(REPORT_PDF)
	rm -rf $(PACKAGE_DIR)
	rm -f $(SUBMIT_ZIP)
	mkdir -p $(PACKAGE_DIR)/Code
	cp -R src $(PACKAGE_DIR)/Code/
	cp Makefile $(PACKAGE_DIR)/Code/Makefile
	cp $(REPORT_PDF) $(PACKAGE_DIR)/report.pdf
	cd $(PACKAGE_DIR) && $(ZIP) -qr ../../$(SUBMIT_ZIP) Code report.pdf
	$(ZIP) -T $(SUBMIT_ZIP)

clean:
	rm -rf $(BUILD_DIR) $(COURSEWORK_DIR)/build
	rm -f $(ROOT_TARGET) $(COURSE_TARGET)
	rm -f $(REPORT_PDF) $(SUBMIT_ZIP)

-include $(wildcard $(OBJ_DIR)/*.d $(OBJ_DIR)/*/*.d $(OBJ_DIR)/*/*/*.d)
