# Giga-RAII

RAII resource management for C. A portable core that builds on every C99
compiler (including MSVC), plus a return-safe layer on GCC/Clang.

```c
#include "raii.h"

int main(void) {
    managed_array(char, buf, 1024) {   // buf is char*, no cast
        strcpy(buf, "Hello, Giga-RAII!");
        printf("%s\n", buf);
    }                                  // free(buf) runs here
    return 0;
}
```

## Why

C has no destructors. Every `malloc` needs a `free`, every `fopen` an `fclose`,
every `open` a `close`. Miss one and you leak. Giga-RAII ties the cleanup to the
scope so you cannot forget it.

## Two tiers

| Tier | Mechanism | Compilers | Return-safe | Shape |
|------|-----------|-----------|-------------|-------|
| **Portable** | for-loop block macros | any C99, incl. MSVC | no (block exit) | `managed(...) { }` |
| **Return-safe** | `__attribute__((cleanup))` | GCC/Clang | yes | `autofree T *x = ...;` |

Write cross-platform code with the portable tier. On GCC/Clang you may also use
the return-safe tier, which runs cleanup on every exit including `return`. Detect
it with `#ifdef RAII_HAS_CLEANUP`.

### Portability

- **Linux / macOS, GCC or Clang:** both tiers, every feature.
- **Windows, MSVC:** portable tier only (MSVC has no cleanup attribute). Needs a
  C99-capable MSVC (VS 2015+).
- **Windows, MinGW/Clang:** both tiers.

The return-safe `autoclose_*` qualifiers for OS resources (fd, dir, pipe, dl,
locale) are POSIX-gated. On Windows, pass the matching destructor
(`closesocket`, `FreeLibrary`, ...) to the portable `managed()` instead.

## Portable tier (every compiler)

### managed(type, name, init, dtor)

Scoped resource; `dtor(name)` runs at block exit. NULL-safe.

```c
managed(char*, buf, malloc(256), free) {
    strcpy(buf, "hello");
}
```

### managed_ok(type, name, init, dtor)

Same, but the block runs only if `init` succeeded (non-NULL), dropping the
`if (!name) break;` guard.

### managed_new / managed_array

Typed allocation, no casts.

```c
managed_new(struct node, n)   { n->next = NULL; }   // n is struct node*
managed_array(double, v, 128) { v[0] = 3.14; }      // v is double*
```

### managed_file / managed_fd

```c
managed_file(f, "data.txt", "r") { fgets(line, sizeof(line), f); }
managed_fd(fd, open("f", O_RDONLY)) { read(fd, buf, sizeof(buf)); }
```

### defer(expr)

Run an expression at block exit; stack for LIFO order.

```c
defer(free(a))
defer(free(b)) { /* ... */ }   // free(b) first, then free(a)
```

### RAII_GUARD(var, acquire, release)

```c
RAII_GUARD(mtx, pthread_mutex_lock, pthread_mutex_unlock) {
    shared_counter++;
}
```

> The block macros clean up at block exit, not on `return` out of the block.
> Keep blocks short, or use the return-safe tier below on GCC/Clang.

## Return-safe tier (GCC/Clang, `RAII_HAS_CLEANUP`)

Declare a variable; cleanup runs on every scope exit, including `return`.

| Qualifier | Variable type | Released with | Availability |
|-----------|---------------|---------------|--------------|
| `autofree` | any pointer | `free` | always |
| `autoclose_file` | `FILE*` | `fclose` | always |
| `autoclose_fd` | `int` (fd) | `close` | POSIX |
| `autoclose_dir` | `DIR*` | `closedir` | POSIX |
| `autoclose_pipe` | `FILE*` | `pclose` | POSIX |
| `autoclose_dl` | `void*` | `dlclose` | POSIX |
| `autoclose_locale` | `locale_t` | `freelocale` | POSIX |

```c
char *load(const char *path) {
    autofree char *buf = malloc(4096);   // freed on every return path
    if (!read_into(path, buf)) return NULL;
    ...
}
```

One `autoclose_fd` covers every fd-returning call: `open`, `creat`, `dup`,
`socket`, `accept`, `mkstemp`, `shm_open`, `epoll_create`, `eventfd`,
`timerfd_create`, `signalfd`, `inotify_init`.

### Custom types

`RAII_CLEANUP_FN` generates a NULL-safe cleanup function; attach it with
`RAII_CLEANUP`.

```c
RAII_CLEANUP_FN(close_db, db_t*, db_close);   // once, at file scope

void use(void) {
    RAII_CLEANUP(close_db) db_t *h = db_open("x");   // db_close(h) at scope exit
}
```

## Building

```sh
make test               # build and run all tests
make example            # build and run the examples
make clean

CC=clang make test
```

Compiler flags: `-Wall -Wextra -Werror -pedantic -std=c99`. Header-only; copy
`include/raii.h` into your tree and `#include "raii.h"`.

## License
This project is licensed under the DBaJ-GPL license. See the [LICENSE](LICENCE) file for details.
