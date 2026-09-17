CC := gcc
FLEX := flex

CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra

BUILD_DIR := build
LEXER_SPEC := src/frontend/lexer.l
LEXER_SOURCE := $(BUILD_DIR)/lexer.c
DRIVER_SOURCE := src/driver/main.c
TARGET := $(BUILD_DIR)/lexer
TEST_SOURCE := tests/lexer/valid/basic.cmm

.PHONY: all clean test

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(LEXER_SOURCE): $(LEXER_SPEC) | $(BUILD_DIR)
	$(FLEX) -o $@ $<

$(TARGET): $(LEXER_SOURCE) $(DRIVER_SOURCE)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(BUILD_DIR)

test: $(TARGET)
	./$(TARGET) $(TEST_SOURCE)
