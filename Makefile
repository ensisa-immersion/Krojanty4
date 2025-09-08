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
	$(CC) $(SRC_DIR)/*.c -o $(BUILD_DR)/krojanty

help:
	@echo "$$HELP_BODY"
