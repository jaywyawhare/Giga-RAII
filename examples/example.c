/**
 * @file example.c
 * @brief Giga-RAII usage examples: portable block tier and return-safe tier.
 */
#define _POSIX_C_SOURCE 200809L

#include "raii.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

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

static void example_portable_block(void)
{
    printf("--- portable: managed / managed_fd / defer ---\n");
    managed(resource_t*, r, resource_open("alpha"), resource_close) {
        if (!r) break;
        printf("  using: %s (refs=%d)\n", r->name, r->refs);
    }
    managed_fd(fd, open("/dev/null", O_RDONLY)) {
        printf("  fd = %d\n", fd);
    }
    char *tmp = malloc(32);
    defer(free(tmp)) {
        strcpy(tmp, "deferred");
        printf("  tmp = \"%s\"\n", tmp);
    }
    printf("\n");
}

#ifdef RAII_HAS_CLEANUP

RAII_CLEANUP_FN(close_resource, resource_t*, resource_close);

static resource_t *example_return_safe(void)
{
    autofree char *scratch = malloc(128);
    strcpy(scratch, "return-safe");
    printf("--- GCC/Clang: autofree / RAII_CLEANUP_FN ---\n");
    printf("  scratch = \"%s\" (freed even on this return)\n", scratch);

    RAII_CLEANUP(close_resource) resource_t *r = resource_open("beta");
    if (r)
        printf("  using: %s\n\n", r->name);
    return NULL;
}

#endif

int main(void)
{
    printf("=== Giga-RAII Examples ===\n\n");
    example_portable_block();
#ifdef RAII_HAS_CLEANUP
    (void)example_return_safe();
#endif
    printf("=== Done ===\n");
    return 0;
}
