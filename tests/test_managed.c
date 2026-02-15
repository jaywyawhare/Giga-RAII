/**
 * @file test_managed.c
 * @brief Tests for managed, managed_file, managed_fd, managed_with.
 */
#include "raii.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

static int g_dtor_called;

static void track_free(void *p)
{
    g_dtor_called++;
    free(p);
}

static int g_fclose_called;

static int track_fclose(FILE *f)
{
    g_fclose_called++;
    return fclose(f);
}

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

static void test_managed_nested(void)
{
    g_dtor_called = 0;
    managed(void*, outer, malloc(32), track_free) {
        assert(outer != NULL);
        managed(void*, inner, malloc(16), track_free) {
            assert(inner != NULL);
        }
        assert(g_dtor_called == 1);
    }
    assert(g_dtor_called == 2);
    printf("  PASS: test_managed_nested\n");
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

static void test_managed_file(void)
{
    g_fclose_called = 0;
    managed(FILE*, f, fopen("/dev/null", "r"), track_fclose) {
        assert(f != NULL);
    }
    assert(g_fclose_called == 1);
    printf("  PASS: test_managed_file\n");
}

static void test_managed_file_null(void)
{
    g_fclose_called = 0;
    managed(FILE*, f, fopen("/no/such/file", "r"), track_fclose) {
        assert(f == NULL);
    }
    assert(g_fclose_called == 0);
    printf("  PASS: test_managed_file_null\n");
}

static void test_managed_file_convenience(void)
{
    managed_file(f, "/dev/null", "r") {
        assert(f != NULL);
    }
    printf("  PASS: test_managed_file_convenience\n");
}

static void test_managed_fd(void)
{
    managed_fd(fd, open("/dev/null", 0)) {
        assert(fd >= 0);
    }
    printf("  PASS: test_managed_fd\n");
}

static void test_managed_fd_invalid(void)
{
    managed_fd(fd, open("/no/such/file", 0)) {
        assert(fd == -1);
    }
    printf("  PASS: test_managed_fd_invalid\n");
}

typedef struct { char name[64]; int open; } fake_db_t;
static int g_db_closed;

static fake_db_t *fake_db_open(const char *name)
{
    fake_db_t *db = (fake_db_t *)malloc(sizeof(fake_db_t));
    if (!db) return NULL;
    strncpy(db->name, name, sizeof(db->name) - 1);
    db->name[sizeof(db->name) - 1] = '\0';
    db->open = 1;
    return db;
}

static void fake_db_close(fake_db_t *db)
{
    db->open = 0;
    g_db_closed++;
    free(db);
}

static void test_managed_with_custom(void)
{
    g_db_closed = 0;
    managed_with(fake_db_t*, db, fake_db_open("testdb"), fake_db_close) {
        assert(db != NULL);
        assert(db->open);
        assert(strcmp(db->name, "testdb") == 0);
    }
    assert(g_db_closed == 1);
    printf("  PASS: test_managed_with_custom\n");
}

int main(void)
{
    printf("test_managed:\n");
    test_managed_basic();
    test_managed_null();
    test_managed_nested();
    test_managed_break();
    test_managed_file();
    test_managed_file_null();
    test_managed_file_convenience();
    test_managed_fd();
    test_managed_fd_invalid();
    test_managed_with_custom();
    printf("All test_managed tests passed.\n\n");
    return 0;
}
