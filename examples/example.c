/**
 * @file example.c
 * @brief Giga-RAII usage examples.
 */
#define _POSIX_C_SOURCE 200809L

#include "raii.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct { char name[64]; int refs; } resource_t;

static resource_t *resource_open(const char *name)
{
    resource_t *r = (resource_t *)malloc(sizeof(resource_t));
    if (!r) return NULL;
    strncpy(r->name, name, sizeof(r->name) - 1);
    r->name[sizeof(r->name) - 1] = '\0';
    r->refs = 1;
    printf("  [resource] opened: %s\n", r->name);
    return r;
}

static void resource_close(resource_t *r)
{
    printf("  [resource] closed: %s\n", r->name);
    free(r);
}

typedef struct { int locked; } simple_lock_t;

static int simple_lock(simple_lock_t *l)   { l->locked = 1; printf("  [lock] acquired\n"); return 0; }
static int simple_unlock(simple_lock_t *l) { l->locked = 0; printf("  [lock] released\n"); return 0; }

static void example_managed(void)
{
    printf("--- managed (malloc/free) ---\n");
    managed(char*, buf, malloc(256), free) {
        strcpy(buf, "hello from managed");
        printf("  buf = \"%s\"\n", buf);
    }
    printf("  (freed)\n\n");
}

static void example_managed_file(void)
{
    printf("--- managed_file ---\n");
    managed_file(f, "/dev/null", "r") {
        printf("  opened /dev/null (FILE* = %p)\n", (void *)f);
    }
    printf("  (closed)\n\n");
}

static void example_managed_fd(void)
{
    printf("--- managed_fd ---\n");
    managed_fd(fd, open("/dev/null", O_RDONLY)) {
        printf("  opened /dev/null as fd %d\n", fd);
    }
    printf("  (closed)\n\n");
}

static void example_managed_custom(void)
{
    printf("--- managed_with (custom type) ---\n");
    managed_with(resource_t*, r, resource_open("my_resource"), resource_close) {
        if (!r) { printf("  open failed\n"); break; }
        printf("  using: %s (refs=%d)\n", r->name, r->refs);
    }
    printf("\n");
}

static void example_defer(void)
{
    printf("--- defer ---\n");
    char *buf = (char *)malloc(256);
    defer(free(buf)) {
        strcpy(buf, "hello from defer");
        printf("  buf = \"%s\"\n", buf);
    }
    printf("  (freed via defer)\n\n");
}

static void example_guard(void)
{
    printf("--- RAII_GUARD ---\n");
    simple_lock_t lk = {0};
    RAII_GUARD(lk, simple_lock, simple_unlock) {
        printf("  critical section (locked=%d)\n", lk.locked);
    }
    printf("  outside (locked=%d)\n\n", lk.locked);
}

static void example_nested(void)
{
    printf("--- nested managed ---\n");
    managed_with(resource_t*, a, resource_open("alpha"), resource_close) {
        if (!a) break;
        managed_with(resource_t*, b, resource_open("beta"), resource_close) {
            if (!b) break;
            printf("  both alive: %s, %s\n", a->name, b->name);
        }
        printf("  only alpha alive\n");
    }
    printf("  both cleaned up\n\n");
}

static void example_managed_malloc(void)
{
    printf("--- managed_malloc ---\n");
    managed_malloc(buf, 256) {
        memset(buf, 'A', 255);
        ((char *)buf)[255] = '\0';
        printf("  allocated 256 bytes, first char = '%c'\n", ((char *)buf)[0]);
    }
    printf("  (freed)\n\n");
}

static void example_managed_dir(void)
{
    printf("--- managed_dir ---\n");
    managed_dir(d, "/tmp") {
        if (!d) { printf("  opendir failed\n"); break; }
        printf("  opened /tmp (DIR* = %p)\n", (void *)d);
        struct dirent *ent;
        int count = 0;
        while ((ent = readdir(d)) != NULL && count < 3) {
            printf("  entry: %s\n", ent->d_name);
            count++;
        }
        if (count == 3) printf("  ... (truncated)\n");
    }
    printf("  (closed)\n\n");
}

static void example_managed_socket(void)
{
    printf("--- managed_socket ---\n");
    managed_socket(fd, AF_INET, SOCK_STREAM, 0) {
        if (fd < 0) { printf("  socket failed\n"); break; }
        printf("  created TCP socket fd=%d\n", fd);
    }
    printf("  (closed)\n\n");
}

int main(void)
{
    printf("=== Giga-RAII Examples ===\n\n");
    example_managed();
    example_managed_file();
    example_managed_fd();
    example_managed_custom();
    example_defer();
    example_guard();
    example_nested();
    example_managed_malloc();
    example_managed_dir();
    example_managed_socket();
    printf("=== Done ===\n");
    return 0;
}
