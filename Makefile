CC       ?= cc
CFLAGS   ?= -Wall -Wextra -Werror -pedantic -std=c99
INCLUDES  = -Iinclude

TEST_DIR  = tests
EX_DIR    = examples
BUILD_DIR = build

TESTS     = $(BUILD_DIR)/test_core $(BUILD_DIR)/test_posix
EXAMPLES  = $(BUILD_DIR)/example

.PHONY: all test example clean

# Header-only: nothing to build by default.
all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ── Tests ──────────────────────────────────────────────────────────────

test: $(TESTS)
	@echo "=== Running tests ==="
	@for t in $(TESTS); do ./$$t || exit 1; done
	@echo "=== All tests passed ==="

$(BUILD_DIR)/test_core: $(TEST_DIR)/test_core.c include/raii.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_DIR)/test_core.c -o $@

$(BUILD_DIR)/test_posix: $(TEST_DIR)/test_posix.c include/raii.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_DIR)/test_posix.c -o $@

# ── Examples ───────────────────────────────────────────────────────────

example: $(EXAMPLES)
	@echo "=== Running example ==="
	./$(BUILD_DIR)/example

$(BUILD_DIR)/example: $(EX_DIR)/example.c include/raii.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(EX_DIR)/example.c -o $@

# ── Clean ──────────────────────────────────────────────────────────────

clean:
	rm -rf $(BUILD_DIR)
