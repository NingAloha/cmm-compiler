CC := gcc
FLEX := flex
BISON := bison

CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra

BUILD_DIR := build
PARSER_SPEC := src/frontend/syntax.y
PARSER_SOURCE := $(BUILD_DIR)/syntax.c
LEXER_SPEC := src/frontend/lexer.l
LEXER_SOURCE := $(BUILD_DIR)/lexer.c
DRIVER_SOURCE := src/driver/main.c
TARGET := $(BUILD_DIR)/parser
TEST_SOURCE := tests/parser/valid/assignment_expression.cmm

.PHONY: all clean test

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(PARSER_SOURCE): $(PARSER_SPEC) | $(BUILD_DIR)
	$(BISON) -d -v -o $@ $<

$(LEXER_SOURCE): $(LEXER_SPEC) $(PARSER_SOURCE) | $(BUILD_DIR)
	$(FLEX) -o $@ $<

$(TARGET): $(PARSER_SOURCE) $(LEXER_SOURCE) $(DRIVER_SOURCE)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	./$(TARGET) $(TEST_SOURCE)
