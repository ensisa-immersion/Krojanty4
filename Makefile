.DEFAULT_GOAL := help

CC := gcc

SRC_DIR := src
BUILD_DR := build
DOCS_DIR := docs

define HELP_BODY
Usage:
  make <command>

Commands:
  docs        Generate documentation using Doxygen.
endef
export HELP_BODY

.PHONY: docs
docs:
	doxygen docs/Doxyfile

compile:
	gcc main.c src/*.c -o ./build/game $(pkg-config --cflags --libs gtk4) -lpthread -I.

help:
	@echo "$$HELP_BODY"
