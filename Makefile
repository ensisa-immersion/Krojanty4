.DEFAULT_GOAL := help

CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O2 -Iinclude -I. $(shell pkg-config --cflags gtk4)
LDFLAGS := -lpthread $(shell pkg-config --libs gtk4)

SRC_DIR := src
BUILD_DIR := build
DOCS_DIR := docs

SRC := $(wildcard $(SRC_DIR)/*.c) main.c
BIN := $(BUILD_DIR)/game

define HELP_BODY
Usage:
  make <command>

Commands:
  compile     Compile the project
  docs        Generate documentation using Doxygen
  clean       Remove build files
endef
export HELP_BODY

.PHONY: docs compile clean help

docs:
	cd $(DOCS_DIR) && doxygen Doxyfile

compile: $(BIN)

$(BIN): $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

clean:
	rm -f $(BUILD_DIR)/*

help:
	@echo "$$HELP_BODY"

