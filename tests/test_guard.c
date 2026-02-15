/**
 * @file test_guard.c
 * @brief Tests for RAII_GUARD.
 */
#include "raii.h"
#include <stdio.h>
#include <assert.h>

typedef struct {
    int locked;
    int lock_count;
    int unlock_count;
} mock_lock_t;

static int mock_lock(mock_lock_t *m)
{
    m->locked = 1;
    m->lock_count++;
    return 0;
}

static int mock_unlock(mock_lock_t *m)
{
    m->locked = 0;
    m->unlock_count++;
    return 0;
}

static void test_guard_basic(void)
{
    mock_lock_t mtx = {0, 0, 0};
    assert(!mtx.locked);
    RAII_GUARD(mtx, mock_lock, mock_unlock) {
        assert(mtx.locked);
        assert(mtx.lock_count == 1);
    }
    assert(!mtx.locked);
    assert(mtx.unlock_count == 1);
    printf("  PASS: test_guard_basic\n");
}

static void test_guard_nested(void)
{
    mock_lock_t a = {0, 0, 0};
    mock_lock_t b = {0, 0, 0};
    RAII_GUARD(a, mock_lock, mock_unlock) {
        assert(a.locked);
        RAII_GUARD(b, mock_lock, mock_unlock) {
            assert(a.locked && b.locked);
        }
        assert(a.locked && !b.locked);
    }
    assert(!a.locked && !b.locked);
    printf("  PASS: test_guard_nested\n");
}

static void test_guard_break(void)
{
    mock_lock_t mtx = {0, 0, 0};
    RAII_GUARD(mtx, mock_lock, mock_unlock) {
        assert(mtx.locked);
        break;
    }
    assert(!mtx.locked);
    assert(mtx.unlock_count == 1);
    printf("  PASS: test_guard_break\n");
}

int main(void)
{
    printf("test_guard:\n");
    test_guard_basic();
    test_guard_nested();
    test_guard_break();
    printf("All test_guard tests passed.\n\n");
    return 0;
}
