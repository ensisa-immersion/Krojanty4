.DEFAULT_GOAL := help

CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O2 -Iinclude -I. $(shell pkg-config --cflags gtk4)
LDFLAGS := -lpthread $(shell pkg-config --libs gtk4)

SRC_DIR := src
BUILD_DIR := build
DOCS_DIR := docs
ASSETS_DIR := assets

# Ressources
GRESOURCE_XML = assets.gresource.xml
GRESOURCE_C = $(BUILDDIR)/resources.c
GRESOURCE_H = $(BUILDDIR)/resources.h



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

.PHONY: docs compile clean help assets

docs:
	cd $(DOCS_DIR) && doxygen Doxyfile

compile:
	gcc main.c src/*.c -o ./build/game $(shell pkg-config --cflags --libs gtk4 librsvg-2.0) -lpthread -Iinclude

$(BIN): $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

# Génération des ressources
assets:
	glib-compile-resources assets.gresource.xml --sourcedir assets --generate

# Compilation des objets
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -I$(BUILDDIR) -c
