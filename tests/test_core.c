/**
 * @file test_core.c
 * @brief Tests for the core cleanup API: autofree, autoclose_file,
 *        managed_new, managed_array, RAII_CLEANUP, RAII_CLEANUP_FN.
 */
#include "raii.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int g_dtor_called;

static int autofree_returns_early(int early)
{
    autofree char *buf = malloc(128);
    assert(buf != NULL);
    strcpy(buf, "scratch");
    if (early)
        return 1;
    return 0;
}

static void test_autofree(void)
{
    assert(autofree_returns_early(1) == 1);
    assert(autofree_returns_early(0) == 0);
    {
        autofree void *p = NULL;
        (void)p;
    }
    printf("  PASS: test_autofree\n");
}

static void test_autoclose_file(void)
{
    autoclose_file FILE *f = fopen("/dev/null", "r");
    assert(f != NULL);
    {
        autoclose_file FILE *bad = fopen("/no/such/file", "r");
        assert(bad == NULL);
    }
    printf("  PASS: test_autoclose_file\n");
}

typedef struct { int a, b; } pair_t;

static void test_managed_new(void)
{
    managed_new(pair_t, p);
    assert(p != NULL);
    p->a = 3;
    p->b = 4;
    assert(p->a + p->b == 7);
    printf("  PASS: test_managed_new\n");
}

static void test_managed_array(void)
{
    managed_array(int, xs, 8);
    assert(xs != NULL);
    for (int i = 0; i < 8; i++)
        xs[i] = i * i;
    assert(xs[7] == 49);
    printf("  PASS: test_managed_array\n");
}

typedef struct { char name[32]; int open; } db_t;

static db_t *db_open(const char *name)
{
    db_t *d = malloc(sizeof(*d));
    if (!d) return NULL;
    strncpy(d->name, name, sizeof(d->name) - 1);
    d->name[sizeof(d->name) - 1] = '\0';
    d->open = 1;
    return d;
}

static void db_close(db_t *d) { d->open = 0; g_dtor_called++; free(d); }

RAII_CLEANUP_FN(close_db, db_t*, db_close);

static void test_cleanup_fn(void)
{
    g_dtor_called = 0;
    {
        RAII_CLEANUP(close_db) db_t *h = db_open("testdb");
        assert(h != NULL && h->open);
        assert(strcmp(h->name, "testdb") == 0);
    }
    assert(g_dtor_called == 1);

    g_dtor_called = 0;
    {
        RAII_CLEANUP(close_db) db_t *h = NULL;
        (void)h;
    }
    assert(g_dtor_called == 0);
    printf("  PASS: test_cleanup_fn\n");
}

int main(void)
{
    printf("test_core:\n");
    test_autofree();
    test_autoclose_file();
    test_managed_new();
    test_managed_array();
    test_cleanup_fn();
    printf("All test_core tests passed.\n\n");
    return 0;
}
