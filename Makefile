CC := gcc
FLEX := flex
BISON := bison
PANDOC := pandoc
WEASYPRINT := weasyprint
ZIP := zip
CLANG_FORMAT := clang-format

CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Isrc/frontend
TEST_STAGES := 01 02
TEST_TARGETS := $(addprefix test-,$(TEST_STAGES))

BUILD_DIR := build
PARSER_SPEC := src/frontend/syntax.y
PARSER_SOURCE := $(BUILD_DIR)/syntax.tab.c
LEXER_SPEC := src/frontend/lexer.l
LEXER_SOURCE := $(BUILD_DIR)/lex.yy.c
DRIVER_SOURCE := src/driver/main.c
TREE_SOURCE := src/frontend/tree.c
TARGET := $(BUILD_DIR)/parser
TEST_RUNNER := tests/run_tests.sh
STAGE_01_TEST_MANIFEST := tests/stage-01/expected.tsv
SEMANTIC_TYPE_SOURCE := src/semantic/type.c
SEMANTIC_SYMBOL_SOURCE := src/semantic/symbol.c
SEMANTIC_ANALYZER_SOURCE := src/semantic/semantic.c
SEMANTIC_UTIL_SOURCE := src/semantic/util.c
STAGE_02_TYPE_TEST_SOURCE := tests/stage-02/test_type.c
STAGE_02_TYPE_TEST := $(BUILD_DIR)/test_type
STAGE_02_SYMBOL_TEST_SOURCE := tests/stage-02/test_symbol.c
STAGE_02_SYMBOL_TEST := $(BUILD_DIR)/test_symbol
STAGE_02_SEMANTIC_MANIFEST := tests/stage-02/semantic_expected.tsv
REPORT_SOURCE := report.md
REPORT_STYLE := report.css
REPORT_HTML := $(BUILD_DIR)/report.html
REPORT_PDF := report.pdf
PACKAGE_DIR := $(BUILD_DIR)/submission
SUBMIT_ZIP := submit.zip

.PHONY: all clean pack format test $(TEST_TARGETS) $(REPORT_PDF)

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(PARSER_SOURCE): $(PARSER_SPEC) | $(BUILD_DIR)
	$(BISON) -d -v -o $@ $<

$(LEXER_SOURCE): $(LEXER_SPEC) $(PARSER_SOURCE) | $(BUILD_DIR)
	$(FLEX) -o $@ $<

$(TARGET): $(PARSER_SOURCE) $(LEXER_SOURCE) $(TREE_SOURCE) $(DRIVER_SOURCE) \
	$(SEMANTIC_ANALYZER_SOURCE) $(SEMANTIC_TYPE_SOURCE) $(SEMANTIC_SYMBOL_SOURCE) \
	$(SEMANTIC_UTIL_SOURCE)
	$(CC) $(CFLAGS) -Isrc/semantic -o $@ $(PARSER_SOURCE) $(TREE_SOURCE) \
		$(DRIVER_SOURCE) $(SEMANTIC_ANALYZER_SOURCE) $(SEMANTIC_TYPE_SOURCE) \
		$(SEMANTIC_SYMBOL_SOURCE) $(SEMANTIC_UTIL_SOURCE)

test: $(TEST_TARGETS)

test-01: $(TARGET) $(TEST_RUNNER) $(STAGE_01_TEST_MANIFEST)
	@sh $(TEST_RUNNER) $(TARGET) $(STAGE_01_TEST_MANIFEST)

test-02: $(STAGE_02_TYPE_TEST) $(STAGE_02_SYMBOL_TEST) $(TARGET) \
	$(TEST_RUNNER) $(STAGE_02_SEMANTIC_MANIFEST)
	@$(STAGE_02_TYPE_TEST)
	@$(STAGE_02_SYMBOL_TEST)
	@sh $(TEST_RUNNER) $(TARGET) $(STAGE_02_SEMANTIC_MANIFEST) --semantic

format:
	@command -v $(CLANG_FORMAT) >/dev/null 2>&1 || { echo "clang-format is not installed. Run: brew install clang-format" >&2; exit 1; }
	find src tests -type f \( -name '*.c' -o -name '*.h' \) -print0 | xargs -0 $(CLANG_FORMAT) -i

$(STAGE_02_TYPE_TEST): $(STAGE_02_TYPE_TEST_SOURCE) $(SEMANTIC_TYPE_SOURCE) $(SEMANTIC_UTIL_SOURCE) src/semantic/type.h src/semantic/util.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/semantic -o $@ $(STAGE_02_TYPE_TEST_SOURCE) $(SEMANTIC_TYPE_SOURCE) $(SEMANTIC_UTIL_SOURCE)

$(STAGE_02_SYMBOL_TEST): $(STAGE_02_SYMBOL_TEST_SOURCE) $(SEMANTIC_TYPE_SOURCE) $(SEMANTIC_SYMBOL_SOURCE) $(SEMANTIC_UTIL_SOURCE) src/semantic/type.h src/semantic/symbol.h src/semantic/util.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Isrc/semantic -o $@ $(STAGE_02_SYMBOL_TEST_SOURCE) $(SEMANTIC_TYPE_SOURCE) $(SEMANTIC_SYMBOL_SOURCE) $(SEMANTIC_UTIL_SOURCE)

$(REPORT_PDF): $(REPORT_SOURCE) $(REPORT_STYLE) | $(BUILD_DIR)
	$(PANDOC) $(REPORT_SOURCE) --standalone --embed-resources --css $(REPORT_STYLE) -o $(REPORT_HTML)
	$(WEASYPRINT) $(REPORT_HTML) $@

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
	rm -rf $(BUILD_DIR)
