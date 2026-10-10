/**
 * @file test_managed.c
 * @brief Tests for the portable block tier (every C99 compiler).
 */
#define _POSIX_C_SOURCE 200809L

#include "raii.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

static int g_dtor_called;
static int g_order[8];
static int g_idx;

static void track_free(void *p) { g_dtor_called++; free(p); }
static void track(int id) { g_order[g_idx++] = id; }

static void test_managed_basic(void)
{
    g_dtor_called = 0;
    managed(void*, buf, malloc(64), track_free) {
        assert(buf != NULL);
        memset(buf, 0xAA, 64);
    }
    assert(g_dtor_called == 1);
    printf("  PASS: test_managed_basic\n");
}

static void test_managed_null(void)
{
    g_dtor_called = 0;
    managed(void*, p, NULL, track_free) {
        assert(p == NULL);
    }
    assert(g_dtor_called == 0);
    printf("  PASS: test_managed_null\n");
}

static void test_managed_break(void)
{
    g_dtor_called = 0;
    managed(void*, buf, malloc(32), track_free) {
        assert(buf != NULL);
        break;
    }
    assert(g_dtor_called == 1);
    printf("  PASS: test_managed_break\n");
}

static void test_managed_ok(void)
{
    int ran = 0;
    managed_ok(void*, p, NULL, free) {
        (void)p;
        ran = 1;
    }
    assert(ran == 0);
    managed_ok(void*, q, malloc(16), free) {
        assert(q != NULL);
        ran = 1;
    }
    assert(ran == 1);
    printf("  PASS: test_managed_ok\n");
}

typedef struct { int a, b; } pair_t;

static void test_managed_new_array(void)
{
    managed_new(pair_t, p) {
        assert(p != NULL);
        p->a = 3;
        p->b = 4;
        assert(p->a + p->b == 7);
    }
    managed_array(int, xs, 8) {
        assert(xs != NULL);
        for (int i = 0; i < 8; i++)
            xs[i] = i * i;
        assert(xs[7] == 49);
    }
    printf("  PASS: test_managed_new_array\n");
}

static void test_managed_file_fd(void)
{
    managed_file(f, "/dev/null", "r") {
        assert(f != NULL);
    }
    managed_fd(fd, open("/dev/null", O_RDONLY)) {
        assert(fd >= 0);
    }
    managed_fd(bad, open("/no/such/file", O_RDONLY)) {
        assert(bad == -1);
    }
    printf("  PASS: test_managed_file_fd\n");
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
    assert(g_order[0] == 3 && g_order[1] == 2 && g_order[2] == 1);
    printf("  PASS: test_defer_lifo\n");
}

typedef struct { int locked, lock_count, unlock_count; } mock_lock_t;
static int mock_lock(mock_lock_t *m)   { m->locked = 1; m->lock_count++; return 0; }
static int mock_unlock(mock_lock_t *m) { m->locked = 0; m->unlock_count++; return 0; }

static void test_guard(void)
{
    mock_lock_t m = {0, 0, 0};
    RAII_GUARD(m, mock_lock, mock_unlock) {
        assert(m.locked && m.lock_count == 1);
    }
    assert(!m.locked && m.unlock_count == 1);
    printf("  PASS: test_guard\n");
}

int main(void)
{
    printf("test_managed:\n");
    test_managed_basic();
    test_managed_null();
    test_managed_break();
    test_managed_ok();
    test_managed_new_array();
    test_managed_file_fd();
    test_defer_lifo();
    test_guard();
    printf("All test_managed tests passed.\n\n");
    return 0;
}
