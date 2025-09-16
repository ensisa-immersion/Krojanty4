.DEFAULT_GOAL := help

CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O2 -Iinclude -I. $(shell pkg-config --cflags gtk4)
LDFLAGS := -lpthread $(shell pkg-config --libs gtk4)

# Flags spéciaux pour les tests (sans GTK)
TEST_CFLAGS := -Wall -Wextra -std=c99 -g -Iinclude -DTEST_BUILD
TEST_LDFLAGS := -lpthread

SRC_DIR := src
BUILD_DIR := build
DOCS_DIR := docs
TEST_DIR := tests

# Sources principales
SRC := $(wildcard $(SRC_DIR)/*.c) main.c
BIN := $(BUILD_DIR)/game

# Sources et objets de test (versions sans GTK)
TEST_SOURCES := $(TEST_DIR)/game_test.c $(TEST_DIR)/move_util_test.c $(SRC_DIR)/logging.c
TEST_OBJECTS := $(BUILD_DIR)/game_test.o $(BUILD_DIR)/move_util_test.o $(BUILD_DIR)/logging_test.o

# Tests exécutables
TESTS := test_game test_move_util test_network test_integration
TEST_EXECUTABLES := $(TESTS:%=$(TEST_DIR)/%)

define HELP_BODY
Usage:
  make <command>

Commands:
  compile        Compile the main project
  docs           Generate documentation using Doxygen
  clean          Remove build files

  Tests:
  tests          Compile and run all tests
  test-clean     Clean test files
endef
export HELP_BODY

.PHONY: docs compile clean help tests test-clean

# ========== COMPILATION PRINCIPALE ==========
compile: $(BIN)

$(BIN): $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

# ========== TESTS ==========
tests: $(BUILD_DIR) $(TEST_EXECUTABLES)
	@for test in $(TESTS); do ./$(TEST_DIR)/$$test || exit 1; done

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compilation des objets de test
$(BUILD_DIR)/game_test.o: $(TEST_DIR)/game_test.c
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(BUILD_DIR)/move_util_test.o: $(TEST_DIR)/move_util_test.c
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(BUILD_DIR)/logging_test.o: $(SRC_DIR)/logging.c
	$(CC) $(TEST_CFLAGS) -c $< -o $@

$(TEST_EXECUTABLES): $(TEST_DIR)/%: $(TEST_DIR)/%.c $(TEST_OBJECTS)
	$(CC) $(TEST_CFLAGS) $< $(filter %.o,$^) -o $@ $(TEST_LDFLAGS)

# Nettoyage des tests
test-clean:
	@echo "🧹 Nettoyage des tests..."
	rm -f $(TEST_EXECUTABLES)
	rm -f $(BUILD_DIR)/*.o

# ========== DOCUMENTATION ==========
docs:
	cd $(DOCS_DIR) && doxygen Doxyfile

# ========== NETTOYAGE ==========
clean:
	@echo "🧹 Nettoyage des fichiers de build..."
	rm -f $(BUILD_DIR)/*

clean-all: clean test-clean
	@echo "🧹 Nettoyage complet terminé"

help:
	@echo "$$HELP_BODY"
