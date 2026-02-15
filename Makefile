CC       ?= cc
CFLAGS   ?= -Wall -Wextra -Werror -pedantic -std=c99
INCLUDES  = -Iinclude

SRC_DIR   = src
TEST_DIR  = tests
EX_DIR    = examples
BUILD_DIR = build

LIB_SRC   = $(SRC_DIR)/raii.c
LIB_OBJ   = $(BUILD_DIR)/raii.o

TESTS     = $(BUILD_DIR)/test_managed $(BUILD_DIR)/test_defer $(BUILD_DIR)/test_guard $(BUILD_DIR)/test_convenience
EXAMPLES  = $(BUILD_DIR)/example

.PHONY: all lib test example clean

all: lib

lib: $(LIB_OBJ)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIB_OBJ): $(LIB_SRC) include/raii.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $(LIB_SRC) -o $@

# ── Tests ──────────────────────────────────────────────────────────────

test: $(TESTS)
	@echo "=== Running tests ==="
	@for t in $(TESTS); do ./$$t || exit 1; done
	@echo "=== All tests passed ==="

$(BUILD_DIR)/test_managed: $(TEST_DIR)/test_managed.c $(LIB_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_DIR)/test_managed.c $(LIB_OBJ) -o $@

$(BUILD_DIR)/test_defer: $(TEST_DIR)/test_defer.c $(LIB_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_DIR)/test_defer.c $(LIB_OBJ) -o $@

$(BUILD_DIR)/test_guard: $(TEST_DIR)/test_guard.c $(LIB_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_DIR)/test_guard.c $(LIB_OBJ) -o $@

$(BUILD_DIR)/test_convenience: $(TEST_DIR)/test_convenience.c $(LIB_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(TEST_DIR)/test_convenience.c $(LIB_OBJ) -o $@

# ── Examples ───────────────────────────────────────────────────────────

example: $(EXAMPLES)
	@echo "=== Running example ==="
	./$(BUILD_DIR)/example

$(BUILD_DIR)/example: $(EX_DIR)/example.c $(LIB_OBJ) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(EX_DIR)/example.c $(LIB_OBJ) -o $@

# ── Clean ──────────────────────────────────────────────────────────────

clean:
	rm -rf $(BUILD_DIR)
