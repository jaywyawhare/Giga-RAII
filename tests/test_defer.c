/**
 * @file test_defer.c
 * @brief Tests for defer.
 */
#include "raii.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int g_order[8];
static int g_idx;

static void track(int id) { g_order[g_idx++] = id; }

static void test_defer_basic(void)
{
    int ran = 0;
    defer(ran = 1) {
        assert(ran == 0);
    }
    assert(ran == 1);
    printf("  PASS: test_defer_basic\n");
}

static void test_defer_lifo(void)
{
    g_idx = 0;
    defer(track(1))
    defer(track(2))
    defer(track(3)) {
        assert(g_idx == 0);
    }
    assert(g_idx == 3);
    assert(g_order[0] == 3);
    assert(g_order[1] == 2);
    assert(g_order[2] == 1);
    printf("  PASS: test_defer_lifo\n");
}

static void test_defer_break(void)
{
    int ran = 0;
    defer(ran = 1) {
        break;
    }
    assert(ran == 1);
    printf("  PASS: test_defer_break\n");
}

static void test_defer_coexist_with_managed(void)
{
    g_idx = 0;
    defer(track(1))
    managed(void*, buf, malloc(32), free) {
        assert(buf != NULL);
        track(99);
    }
    assert(g_order[0] == 99);
    assert(g_order[1] == 1);
    printf("  PASS: test_defer_coexist_with_managed\n");
}

int main(void)
{
    printf("test_defer:\n");
    test_defer_basic();
    test_defer_lifo();
    test_defer_break();
    test_defer_coexist_with_managed();
    printf("All test_defer tests passed.\n\n");
    return 0;
}
