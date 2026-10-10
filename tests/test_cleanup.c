/**
 * @file test_cleanup.c
 * @brief Tests for the return-safe tier (GCC/Clang, RAII_HAS_CLEANUP).
 */
#define _POSIX_C_SOURCE 200809L

#include "raii.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef RAII_HAS_CLEANUP

#include <fcntl.h>
#include <sys/socket.h>

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

static void test_autoclose_fd(void)
{
    autoclose_fd int fd = open("/dev/null", O_RDONLY);
    autoclose_fd int sk = socket(AF_INET, SOCK_STREAM, 0);
    assert(fd >= 0 && sk >= 0);
    {
        autoclose_fd int bad = open("/no/such/file", O_RDONLY);
        assert(bad == -1);
    }
    printf("  PASS: test_autoclose_fd\n");
}

static void test_autoclose_dir_pipe(void)
{
    autoclose_dir DIR *d = opendir("/tmp");
    assert(d != NULL);
    autoclose_pipe FILE *p = popen("echo hi", "r");
    assert(p != NULL);
    char buf[16] = {0};
    assert(fgets(buf, sizeof(buf), p) != NULL);
    assert(strncmp(buf, "hi", 2) == 0);
    printf("  PASS: test_autoclose_dir_pipe\n");
}

typedef struct { int open; } db_t;
static db_t *db_open(void) { db_t *d = malloc(sizeof(*d)); if (d) d->open = 1; return d; }
static void db_close(db_t *d) { g_dtor_called++; free(d); }

RAII_CLEANUP_FN(close_db, db_t*, db_close);

static void test_cleanup_fn(void)
{
    g_dtor_called = 0;
    {
        RAII_CLEANUP(close_db) db_t *h = db_open();
        assert(h != NULL && h->open);
    }
    assert(g_dtor_called == 1);
    {
        RAII_CLEANUP(close_db) db_t *h = NULL;
        (void)h;
    }
    assert(g_dtor_called == 1);
    printf("  PASS: test_cleanup_fn\n");
}

int main(void)
{
    printf("test_cleanup:\n");
    test_autofree();
    test_autoclose_file();
    test_autoclose_fd();
    test_autoclose_dir_pipe();
    test_cleanup_fn();
    printf("All test_cleanup tests passed.\n\n");
    return 0;
}

#else /* !RAII_HAS_CLEANUP */

int main(void)
{
    printf("test_cleanup:\n  SKIP: compiler lacks __attribute__((cleanup))\n\n");
    return 0;
}

#endif
