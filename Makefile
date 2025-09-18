.DEFAULT_GOAL := game

CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O2 -Iinclude -I. $(shell pkg-config --cflags gtk4)
LDFLAGS := -lpthread $(shell pkg-config --libs gtk4)

# Flags spéciaux pour les tests (sans GTK)
TEST_CFLAGS := -Wall -Wextra -std=c99 -g -Iinclude -DTEST_BUILD
TEST_LDFLAGS := -lpthread

# Flags pour la couverture de code
COVERAGE_CFLAGS := $(TEST_CFLAGS) -fprofile-arcs -ftest-coverage $(shell pkg-config --cflags gtk4)
COVERAGE_LDFLAGS := $(TEST_LDFLAGS) -lgcov $(shell pkg-config --libs gtk4)

SRC_DIR := src
BUILD_DIR := build
DOCS_DIR := docs
TEST_DIR := tests
COVERAGE_DIR := coverage

# Sources principales
SRC := $(wildcard $(SRC_DIR)/*.c) main.c
BIN := $(BUILD_DIR)/game

# Sources et objets de test (versions sans GTK)
TEST_SOURCES := $(TEST_DIR)/game_test.c $(TEST_DIR)/move_util_test.c $(SRC_DIR)/logging.c
TEST_OBJECTS := $(BUILD_DIR)/game_test.o $(BUILD_DIR)/move_util_test.o $(BUILD_DIR)/logging_test.o

# Tests exécutables
TESTS := test_game test_move_util test_network test_integration
TEST_EXECUTABLES := $(TESTS:%=$(TEST_DIR)/%)

# Objects de test avec couverture
COVERAGE_OBJECTS := $(BUILD_DIR)/coverage_game_test.o $(BUILD_DIR)/coverage_move_util_test.o $(BUILD_DIR)/coverage_logging_test.o
COVERAGE_EXECUTABLES := $(TESTS:%=$(TEST_DIR)/coverage_%)

define HELP_BODY
Usage:
  make <command>

Commands:
  game           Compile the main project
  clean          Remove build files
  clean-all      Remove all generated files (build, tests, docs, coverage)

  Docs:
  docs 			Generate documentation using Doxygen
  docs-clean    Remove generated documentation files

  Tests:
  tests          Compile and run all tests
  test-clean     Clean test files

  Coverage:
  coverage       Generate code coverage report with lcov
  coverage-clean Clean coverage files

  Logs:
  logs-clean	Remove log files
endef
export HELP_BODY

.PHONY: docs compile clean help tests test-clean game coverage coverage-clean

# ========== COMPILATION PRINCIPALE ==========
game: $(BIN)

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
	rm -f $(TEST_EXECUTABLES)
	rm -f $(BUILD_DIR)/*.o

# ========== DOCUMENTATION ==========
docs:
	cd $(DOCS_DIR) && doxygen Doxyfile

docs-clean:
	rm -rf $(DOCS_DIR)/output

# ========== NETTOYAGE ==========
clean:
	rm -f $(BUILD_DIR)/*

clean-all: clean test-clean docs-clean

help:
	@echo "$$HELP_BODY"

# ========== LOGS ==========
logs-clean:
	rm -f logs/*.log

# ========== COUVERTURE DE CODE ==========
coverage: $(BUILD_DIR)
	@mkdir -p $(COVERAGE_DIR)
	@for test_file in $(wildcard $(TEST_DIR)/test_*.c); do \
		test_name=$$(basename $$test_file .c); \
		$(CC) $(COVERAGE_CFLAGS) $$test_file $(filter-out main.c,$(SRC)) -o $(BUILD_DIR)/$$test_name $(COVERAGE_LDFLAGS) && \
		./$(BUILD_DIR)/$$test_name || echo "Test $$test_name a échoué (on continue quand même)"; \
	done
	lcov --capture --directory $(BUILD_DIR) --base-directory . --output-file $(COVERAGE_DIR)/coverage.info
	lcov --remove $(COVERAGE_DIR)/coverage.info 'tests/*' --output-file $(COVERAGE_DIR)/coverage_filtered.info
	genhtml $(COVERAGE_DIR)/coverage_filtered.info --output-directory $(COVERAGE_DIR)/html

coverage-clean:
	rm -rf $(COVERAGE_DIR)
	rm -f $(BUILD_DIR)/*.gcno $(BUILD_DIR)/*.gcda $(BUILD_DIR)/game_coverage
	rm -f $(BUILD_DIR)/test_*
