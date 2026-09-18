CC := gcc
FLEX := flex
BISON := bison

CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Isrc/frontend

BUILD_DIR := build
PARSER_SPEC := src/frontend/syntax.y
PARSER_SOURCE := $(BUILD_DIR)/syntax.tab.c
LEXER_SPEC := src/frontend/lexer.l
LEXER_SOURCE := $(BUILD_DIR)/lex.yy.c
DRIVER_SOURCE := src/driver/main.c
TREE_SOURCE := src/frontend/tree.c
TARGET := $(BUILD_DIR)/parser
VALID_TESTS := $(sort $(shell find tests/parser/valid -type f -name '*.cmm'))
INVALID_TESTS := $(sort $(shell find tests/parser/invalid -type f -name '*.cmm'))

.PHONY: all clean test

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(PARSER_SOURCE): $(PARSER_SPEC) | $(BUILD_DIR)
	$(BISON) -d -v -o $@ $<

$(LEXER_SOURCE): $(LEXER_SPEC) $(PARSER_SOURCE) | $(BUILD_DIR)
	$(FLEX) -o $@ $<

$(TARGET): $(PARSER_SOURCE) $(LEXER_SOURCE) $(TREE_SOURCE) $(DRIVER_SOURCE)
	$(CC) $(CFLAGS) -o $@ $(PARSER_SOURCE) $(TREE_SOURCE) $(DRIVER_SOURCE)

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	@passed=0; total=0; failed=0; \
	for test in $(VALID_TESTS); do \
		total=$$((total + 1)); \
		if ./$(TARGET) "$$test" >/dev/null 2>&1; then \
			passed=$$((passed + 1)); \
		else \
			echo "FAIL (expected acceptance) $$test"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	for test in $(INVALID_TESTS); do \
		total=$$((total + 1)); \
		if ./$(TARGET) "$$test" >/dev/null 2>&1; then \
			echo "FAIL (expected rejection) $$test"; \
			failed=$$((failed + 1)); \
		else \
			passed=$$((passed + 1)); \
		fi; \
	done; \
	echo "$$passed/$$total tests passed"; \
	test $$failed -eq 0
