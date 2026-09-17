CC := gcc
FLEX := flex
BISON := bison

CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra

BUILD_DIR := build
PARSER_SPEC := src/frontend/syntax.y
PARSER_SOURCE := $(BUILD_DIR)/syntax.tab.c
LEXER_SPEC := src/frontend/lexer.l
LEXER_SOURCE := $(BUILD_DIR)/lexer.yy.c
DRIVER_SOURCE := src/driver/main.c
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

$(TARGET): $(PARSER_SOURCE) $(LEXER_SOURCE) $(DRIVER_SOURCE)
	$(CC) $(CFLAGS) -o $@ $(PARSER_SOURCE) $(DRIVER_SOURCE)

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	@for test in $(VALID_TESTS); do \
		if ./$(TARGET) "$$test" >/dev/null; then \
			echo "PASS $$test"; \
		else \
			echo "FAIL $$test"; exit 1; \
		fi; \
	done
	@for test in $(INVALID_TESTS); do \
		if ./$(TARGET) "$$test" >/dev/null 2>&1; then \
			echo "FAIL (expected rejection) $$test"; exit 1; \
		else \
			echo "PASS (rejected) $$test"; \
		fi; \
	done
