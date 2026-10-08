/**
 * @file example.c
 * @brief Giga-RAII usage examples (cleanup-attribute API).
 */
#define _POSIX_C_SOURCE 200809L

#include "raii.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/socket.h>

typedef struct { char name[64]; int refs; } resource_t;

static resource_t *resource_open(const char *name)
{
    resource_t *r = malloc(sizeof(*r));
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

RAII_CLEANUP_FN(close_resource, resource_t*, resource_close);

static void example_autofree(void)
{
    printf("--- autofree ---\n");
    autofree char *buf = malloc(256);
    strcpy(buf, "hello from autofree");
    printf("  buf = \"%s\"\n", buf);
    printf("  (freed at scope exit, even on early return)\n\n");
}

static void example_typed_alloc(void)
{
    printf("--- managed_new / managed_array ---\n");
    managed_new(resource_t, r);
    strcpy(r->name, "typed");
    r->refs = 2;
    printf("  r->name = \"%s\", refs = %d\n\n", r->name, r->refs);
}

static void example_file(void)
{
    printf("--- autoclose_file ---\n");
    autoclose_file FILE *f = fopen("/dev/null", "r");
    printf("  opened /dev/null (FILE* = %p)\n\n", (void *)f);
}

static void example_fd(void)
{
    printf("--- autoclose_fd ---\n");
    autoclose_fd int fd = open("/dev/null", O_RDONLY);
    autoclose_fd int sk = socket(AF_INET, SOCK_STREAM, 0);
    printf("  fd=%d, socket=%d (both closed at scope exit)\n\n", fd, sk);
}

static void example_dir(void)
{
    printf("--- autoclose_dir ---\n");
    autoclose_dir DIR *d = opendir("/tmp");
    struct dirent *ent;
    int count = 0;
    while (d && (ent = readdir(d)) != NULL && count < 3) {
        printf("  entry: %s\n", ent->d_name);
        count++;
    }
    printf("\n");
}

static void example_custom(void)
{
    printf("--- RAII_CLEANUP_FN (custom type) ---\n");
    RAII_CLEANUP(close_resource) resource_t *r = resource_open("my_resource");
    if (r)
        printf("  using: %s (refs=%d)\n\n", r->name, r->refs);
}

int main(void)
{
    printf("=== Giga-RAII Examples ===\n\n");
    example_autofree();
    example_typed_alloc();
    example_file();
    example_fd();
    example_dir();
    example_custom();
    printf("=== Done ===\n");
    return 0;
}
