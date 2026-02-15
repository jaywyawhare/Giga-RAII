/**
 * @file test_convenience.c
 * @brief Tests for convenience macros (C99, POSIX, Linux).
 */
#define _POSIX_C_SOURCE 200809L

#include "raii.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void test_managed_malloc(void)
{
    managed_malloc(buf, 128) {
        assert(buf != NULL);
        memset(buf, 0xAB, 128);
        assert(((unsigned char *)buf)[0] == 0xAB);
        assert(((unsigned char *)buf)[127] == 0xAB);
    }
    printf("  PASS: test_managed_malloc\n");
}

static void test_managed_calloc(void)
{
    managed_calloc(buf, 16, sizeof(int)) {
        assert(buf != NULL);
        int *arr = (int *)buf;
        int i;
        for (i = 0; i < 16; i++)
            assert(arr[i] == 0);
        arr[0] = 42;
        assert(arr[0] == 42);
    }
    printf("  PASS: test_managed_calloc\n");
}

static void test_managed_tmpfile(void)
{
    managed_tmpfile(f) {
        assert(f != NULL);
        fprintf(f, "hello");
        rewind(f);
        char buf[16] = {0};
        assert(fgets(buf, sizeof(buf), f) != NULL);
        assert(strcmp(buf, "hello") == 0);
    }
    printf("  PASS: test_managed_tmpfile\n");
}

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

static void test_managed_aligned_alloc(void)
{
    managed_aligned_alloc(buf, 64, 256) {
        assert(buf != NULL);
        assert(((size_t)buf % 64) == 0);
        memset(buf, 0xCC, 256);
    }
    printf("  PASS: test_managed_aligned_alloc\n");
}

#endif

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L

#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>

static void test_managed_strdup(void)
{
    managed_strdup(s, "hello world") {
        assert(s != NULL);
        assert(strcmp(s, "hello world") == 0);
    }
    printf("  PASS: test_managed_strdup\n");
}

static void test_managed_strndup(void)
{
    managed_strndup(s, "hello world", 5) {
        assert(s != NULL);
        assert(strcmp(s, "hello") == 0);
    }
    printf("  PASS: test_managed_strndup\n");
}

static void test_managed_dir(void)
{
    managed_dir(d, "/tmp") {
        assert(d != NULL);
    }
    printf("  PASS: test_managed_dir\n");
}

static void test_managed_fdopen(void)
{
    int rawfd = open("/dev/null", O_RDONLY);
    assert(rawfd >= 0);
    managed_fdopen(f, rawfd, "r") {
        assert(f != NULL);
    }
    printf("  PASS: test_managed_fdopen\n");
}

static void test_managed_popen(void)
{
    managed_popen(p, "echo hello", "r") {
        assert(p != NULL);
        char buf[64] = {0};
        assert(fgets(buf, sizeof(buf), p) != NULL);
        buf[strcspn(buf, "\n")] = '\0';
        assert(strcmp(buf, "hello") == 0);
    }
    printf("  PASS: test_managed_popen\n");
}

static void test_managed_creat(void)
{
    char tmpl[] = "/tmp/giga_raii_test_creat_XXXXXX";
    int tmpfd = mkstemp(tmpl);
    assert(tmpfd >= 0);
    close(tmpfd);
    unlink(tmpl);

    managed_creat(fd, tmpl, 0644) {
        assert(fd >= 0);
        assert(write(fd, "hi", 2) == 2);
    }
    unlink(tmpl);
    printf("  PASS: test_managed_creat\n");
}

static void test_managed_dup(void)
{
    int orig = open("/dev/null", O_RDONLY);
    assert(orig >= 0);
    managed_dup(fd, orig) {
        assert(fd >= 0);
        assert(fd != orig);
    }
    close(orig);
    printf("  PASS: test_managed_dup\n");
}

static void test_managed_socket(void)
{
    managed_socket(fd, AF_INET, SOCK_STREAM, 0) {
        assert(fd >= 0);
    }
    printf("  PASS: test_managed_socket\n");
}

static void test_managed_mkstemp(void)
{
    char tmpl[] = "/tmp/giga_raii_test_XXXXXX";
    managed_mkstemp(fd, tmpl) {
        assert(fd >= 0);
        assert(write(fd, "test", 4) == 4);
    }
    unlink(tmpl);
    printf("  PASS: test_managed_mkstemp\n");
}

#endif

#if defined(__linux__)

#include <sys/epoll.h>
#include <signal.h>

static void test_managed_epoll(void)
{
    managed_epoll(fd, 1) {
        assert(fd >= 0);
    }
    printf("  PASS: test_managed_epoll\n");
}

static void test_managed_epoll1(void)
{
    managed_epoll1(fd, 0) {
        assert(fd >= 0);
    }
    printf("  PASS: test_managed_epoll1\n");
}

static void test_managed_eventfd(void)
{
    managed_eventfd(fd, 0, 0) {
        assert(fd >= 0);
    }
    printf("  PASS: test_managed_eventfd\n");
}

static void test_managed_timerfd(void)
{
    managed_timerfd(fd, CLOCK_MONOTONIC, 0) {
        assert(fd >= 0);
    }
    printf("  PASS: test_managed_timerfd\n");
}

static void test_managed_signalfd(void)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    managed_signalfd(fd, -1, &mask, 0) {
        assert(fd >= 0);
    }
    printf("  PASS: test_managed_signalfd\n");
}

static void test_managed_inotify(void)
{
    managed_inotify(fd) {
        assert(fd >= 0);
    }
    printf("  PASS: test_managed_inotify\n");
}

static void test_managed_inotify1(void)
{
    managed_inotify1(fd, 0) {
        assert(fd >= 0);
    }
    printf("  PASS: test_managed_inotify1\n");
}

#endif

int main(void)
{
    printf("test_convenience:\n");

    test_managed_malloc();
    test_managed_calloc();
    test_managed_tmpfile();

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    test_managed_aligned_alloc();
#endif

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
    test_managed_strdup();
    test_managed_strndup();
    test_managed_dir();
    test_managed_fdopen();
    test_managed_popen();
    test_managed_creat();
    test_managed_dup();
    test_managed_socket();
    test_managed_mkstemp();
#endif

#if defined(__linux__)
    test_managed_epoll();
    test_managed_epoll1();
    test_managed_eventfd();
    test_managed_timerfd();
    test_managed_signalfd();
    test_managed_inotify();
    test_managed_inotify1();
#endif

    printf("All test_convenience tests passed.\n\n");
    return 0;
}
