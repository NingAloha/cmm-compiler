CC := gcc
FLEX := flex
BISON := bison
PANDOC := pandoc
WEASYPRINT := weasyprint
ZIP := zip

CFLAGS := -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Isrc/frontend

BUILD_DIR := build
PARSER_SPEC := src/frontend/syntax.y
PARSER_SOURCE := $(BUILD_DIR)/syntax.tab.c
LEXER_SPEC := src/frontend/lexer.l
LEXER_SOURCE := $(BUILD_DIR)/lex.yy.c
DRIVER_SOURCE := src/driver/main.c
TREE_SOURCE := src/frontend/tree.c
TARGET := $(BUILD_DIR)/parser
REPORT_SOURCE := report.md
REPORT_STYLE := report.css
REPORT_HTML := $(BUILD_DIR)/report.html
REPORT_PDF := report.pdf
PACKAGE_DIR := $(BUILD_DIR)/submission
SUBMIT_ZIP := submit.zip

.PHONY: all clean pack $(REPORT_PDF)

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(PARSER_SOURCE): $(PARSER_SPEC) | $(BUILD_DIR)
	$(BISON) -d -v -o $@ $<

$(LEXER_SOURCE): $(LEXER_SPEC) $(PARSER_SOURCE) | $(BUILD_DIR)
	$(FLEX) -o $@ $<

$(TARGET): $(PARSER_SOURCE) $(LEXER_SOURCE) $(TREE_SOURCE) $(DRIVER_SOURCE)
	$(CC) $(CFLAGS) -o $@ $(PARSER_SOURCE) $(TREE_SOURCE) $(DRIVER_SOURCE)

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
