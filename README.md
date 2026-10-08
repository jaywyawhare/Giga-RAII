# Giga-RAII

RAII resource management for C, built on `__attribute__((cleanup))`. Declare a
variable, and it is released automatically when it goes out of scope.

```c
#include "raii.h"

int main(void) {
    managed_array(char, buf, 1024);   // buf is char*, no cast
    strcpy(buf, "Hello, Giga-RAII!");
    printf("%s\n", buf);
    return 0;                         // free(buf) runs here
}
```

## Why

C has no destructors. Every `malloc` needs a `free`, every `fopen` an `fclose`,
every `open` a `close`. Miss one and you leak. Giga-RAII attaches the cleanup to
the variable itself, so it runs on **every** path out of the scope: `return`,
`break`, `goto`, or falling off the end.

- **Header-only.** Drop in `include/raii.h`, nothing to compile or link
- **No footguns.** Cleanup runs even on `return` (unlike block/`for`-loop tricks)
- **No casts.** Typed allocation helpers bind the right pointer type
- 11 tests on GCC and Clang with `-Wall -Wextra -Werror -pedantic`

> **Requires GCC or Clang.** Giga-RAII uses the `cleanup` attribute, which MSVC
> does not support. The header `#error`s on unsupported compilers.

## Quick Start

```sh
git clone https://github.com/jaywyawhare/Giga-RAII.git
cd Giga-RAII
make test
```

To use it, copy `include/raii.h` into your tree and `#include "raii.h"`. That's it.

## API

### Built-in qualifiers

Put these in front of a variable declaration; the resource is released at scope exit.

| Qualifier | Variable type | Released with | Availability |
|-----------|---------------|---------------|--------------|
| `autofree` | any pointer | `free` | always |
| `autoclose_file` | `FILE*` | `fclose` | always |
| `autoclose_fd` | `int` (fd) | `close` (if `>= 0`) | POSIX |
| `autoclose_dir` | `DIR*` | `closedir` | POSIX |
| `autoclose_pipe` | `FILE*` | `pclose` | POSIX |
| `autoclose_dl` | `void*` | `dlclose` | POSIX |
| `autoclose_locale` | `locale_t` | `freelocale` | POSIX |

```c
autofree       char *s  = strdup("hi");               // free
autoclose_file FILE *f  = fopen("data.txt", "r");     // fclose
autoclose_fd   int   fd = socket(AF_INET, SOCK_STREAM, 0);  // close
autoclose_dir  DIR  *d  = opendir("/tmp");            // closedir
```

All are NULL/sentinel safe: a failed `fopen` (NULL) or `open` (`-1`) is not closed.
One `autoclose_fd` covers every fd-returning call: `open`, `creat`, `dup`,
`socket`, `accept`, `mkstemp`, `shm_open`, `epoll_create`, `eventfd`,
`timerfd_create`, `signalfd`, `inotify_init`, ...

### Typed allocation

| Macro | Binds | Released with |
|-------|-------|---------------|
| `managed_new(T, name)` | `T *name = malloc(sizeof(T))` | `free` |
| `managed_array(T, name, n)` | `T *name = malloc(sizeof(T) * n)` | `free` |

```c
managed_new(struct node, n);   // n is struct node*
managed_array(double, v, 128);  // v is double*, no cast
```

### Custom types

`RAII_CLEANUP_FN` generates a NULL-safe cleanup function for your type; attach it
with `RAII_CLEANUP`.

```c
RAII_CLEANUP_FN(close_db, db_t*, db_close);   // once, at file scope

void use(void) {
    RAII_CLEANUP(close_db) db_t *h = db_open("x");   // db_close(h) at scope exit
    ...
}
```

For a one-off cleanup, point `RAII_CLEANUP(fn)` at any `void fn(T*)` you write.

## Limitations

- **GCC/Clang only.** Relies on `__attribute__((cleanup))`.
- **Cleanup runs at the enclosing scope's exit.** Open a nested `{ }` block if you
  want a resource released earlier than the end of the function.
- **Order is reverse of declaration** within a scope (last declared, first released).
- **Not thread-safe by itself.** The qualifiers manage scope, not synchronization.

## Building

```sh
make test               # build and run all tests
make example            # build and run the examples
make clean              # remove build artifacts

CC=clang make test      # test with clang
```

Compiler flags: `-Wall -Wextra -Werror -pedantic -std=c99`

## License
This project is licensed under the DBaJ-GPL license. See the [LICENSE](LICENCE) file for details.
