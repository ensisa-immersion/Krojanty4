.DEFAULT_GOAL := game

CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -O2 -Iinclude -I. $(shell pkg-config --cflags gtk4)
LDFLAGS := -lpthread $(shell pkg-config --libs gtk4)

TEST_CFLAGS := -Wall -Wextra -std=c11 -O2 -Iinclude -I.
TEST_LDFLAGS := -lpthread

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

  Logs:
  logs-clean	Remove log files
endef
export HELP_BODY

.PHONY: docs compile clean help tests test-clean game

game: $(BIN)

$(BIN): $(SRC)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

docs:
	cd $(DOCS_DIR) && doxygen Doxyfile

docs-clean:
	rm -rf $(DOCS_DIR)/output

clean:
	rm -f $(BUILD_DIR)/*

clean-all: clean tests-clean docs-clean logs-clean

help:
	@echo "$$HELP_BODY"

logs-clean:
	rm -f logs/*.log

tests: $(BUILD_DIR)
	@mkdir -p $(COVERAGE_DIR)
	@for test_file in $(wildcard $(TEST_DIR)/test_*.c); do \
		test_name=$$(basename $$test_file .c); \
		$(CC) $(COVERAGE_CFLAGS) $$test_file $(filter-out main.c,$(SRC)) -o $(BUILD_DIR)/$$test_name $(COVERAGE_LDFLAGS) && \
		./$(BUILD_DIR)/$$test_name; \
	done
	lcov --capture --directory $(BUILD_DIR) --base-directory . --output-file $(COVERAGE_DIR)/coverage.info
	lcov --remove $(COVERAGE_DIR)/coverage.info 'tests/*' '/usr/*' --output-file $(COVERAGE_DIR)/coverage_filtered.info
	genhtml $(COVERAGE_DIR)/coverage_filtered.info --output-directory $(COVERAGE_DIR)/html

tests-clean:
	rm -rf $(COVERAGE_DIR)/* $(BUILD_DIR)/*.gcno $(BUILD_DIR)/*.gcda $(BUILD_DIR)/game_coverage $(BUILD_DIR)/*.o$(BUILD_DIR)/test_*
