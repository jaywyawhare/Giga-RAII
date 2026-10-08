/**
 * @file test_posix.c
 * @brief Tests for the POSIX cleanup qualifiers (autoclose_fd/dir/pipe/...).
 */
#define _POSIX_C_SOURCE 200809L

#include "raii.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>

static void test_autoclose_fd(void)
{
    autoclose_fd int fd = open("/dev/null", O_RDONLY);
    assert(fd >= 0);
    {
        autoclose_fd int bad = open("/no/such/file", O_RDONLY);
        assert(bad == -1);
    }
    printf("  PASS: test_autoclose_fd\n");
}

static void test_autoclose_fd_socket(void)
{
    autoclose_fd int s = socket(AF_INET, SOCK_STREAM, 0);
    assert(s >= 0);
    printf("  PASS: test_autoclose_fd_socket\n");
}

static void test_autoclose_fd_mkstemp(void)
{
    char tmpl[] = "/tmp/raii_XXXXXX";
    autoclose_fd int fd = mkstemp(tmpl);
    assert(fd >= 0);
    unlink(tmpl);
    printf("  PASS: test_autoclose_fd_mkstemp\n");
}

static void test_autoclose_dir(void)
{
    autoclose_dir DIR *d = opendir("/tmp");
    assert(d != NULL);
    printf("  PASS: test_autoclose_dir\n");
}

static void test_autofree_strdup(void)
{
    autofree char *s = strdup("hello");
    assert(s != NULL && strcmp(s, "hello") == 0);
    printf("  PASS: test_autofree_strdup\n");
}

static void test_autoclose_pipe(void)
{
    autoclose_pipe FILE *p = popen("echo hi", "r");
    assert(p != NULL);
    char buf[16] = {0};
    assert(fgets(buf, sizeof(buf), p) != NULL);
    assert(strncmp(buf, "hi", 2) == 0);
    printf("  PASS: test_autoclose_pipe\n");
}

int main(void)
{
    printf("test_posix:\n");
    test_autoclose_fd();
    test_autoclose_fd_socket();
    test_autoclose_fd_mkstemp();
    test_autoclose_dir();
    test_autofree_strdup();
    test_autoclose_pipe();
    printf("All test_posix tests passed.\n\n");
    return 0;
}
