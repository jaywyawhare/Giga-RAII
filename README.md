# Giga-RAII

Portable RAII resource management for C. Pure C99. No compiler extensions.

```c
#include "raii.h"

int main(void) {
    managed_malloc(buf, 1024) {
        strcpy(buf, "Hello, Giga-RAII!");
        printf("%s\n", (char *)buf);
    }
    // free(buf) called automatically
    return 0;
}
```

## Why

C has no destructors. Every `malloc` needs a `free`, every `fopen` needs an `fclose`, every `open` needs a `close`. Miss one and you leak. Giga-RAII fixes this with scoped blocks that clean up automatically when execution leaves the block.

- No `__attribute__((cleanup))` -- works on MSVC too
- No GCC/Clang-specific anything -- pure ISO C99
- Single header + one tiny `.c` file
- 36 tests, verified on both GCC and Clang with `-Wall -Wextra -Werror -pedantic`

## Quick Start

```sh
git clone https://github.com/jaywyawhara/Giga-RAII.git
cd Giga-RAII
make test
```

To use in your project, copy `include/raii.h` and `src/raii.c` into your source tree and compile `raii.c` alongside your code.

## Core API

### managed(type, name, init, dtor)

Scoped resource. Calls `dtor(name)` at block exit. NULL-safe -- if `init` returns NULL, `dtor` is not called.

```c
managed(char*, buf, malloc(256), free) {
    strcpy(buf, "hello");
}
// free(buf) runs here
```

### managed_file(name, path, mode)

Scoped `fopen`/`fclose`.

```c
managed_file(f, "data.txt", "r") {
    fgets(line, sizeof(line), f);
}
// fclose(f) runs here
```

### managed_fd(name, open_expr)

Scoped file descriptor. Uses -1 as invalid sentinel.

```c
managed_fd(fd, open("file.txt", O_RDONLY)) {
    read(fd, buf, sizeof(buf));
}
// close(fd) runs here
```

### managed_with(type, name, init, dtor)

Alias for `managed`. Reads better with custom types.

```c
managed_with(SSL*, ssl, SSL_new(ctx), SSL_free) {
    SSL_connect(ssl);
}
```

### defer(expr)

Run an expression at block exit. Stack multiple for LIFO order.

```c
char *a = malloc(100);
char *b = malloc(200);
defer(free(a))
defer(free(b)) {
    // use a and b
}
// free(b) runs first, then free(a)
```

### RAII_GUARD(var, acquire, release)

Scoped lock/unlock on an existing variable.

```c
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
RAII_GUARD(mtx, pthread_mutex_lock, pthread_mutex_unlock) {
    shared_counter++;
}
// pthread_mutex_unlock(&mtx) runs here
```

## Convenience Macros

Built-in wrappers so you never spell the destructor for common resource types.

### C99 (always available)

| Macro | Acquires | Releases |
|-------|----------|----------|
| `managed_malloc(name, size)` | `malloc` | `free` |
| `managed_calloc(name, nmemb, size)` | `calloc` | `free` |
| `managed_tmpfile(name)` | `tmpfile` | `fclose` |

### C11 (when `__STDC_VERSION__ >= 201112L`)

| Macro | Acquires | Releases |
|-------|----------|----------|
| `managed_aligned_alloc(name, align, size)` | `aligned_alloc` | `free` |

### POSIX (when `_POSIX_C_SOURCE >= 200112L`)

| Macro | Acquires | Releases |
|-------|----------|----------|
| `managed_strdup(name, s)` | `strdup` | `free` |
| `managed_strndup(name, s, n)` | `strndup` | `free` |
| `managed_dir(name, path)` | `opendir` | `closedir` |
| `managed_fdopendir(name, fd)` | `fdopendir` | `closedir` |
| `managed_fdopen(name, fd, mode)` | `fdopen` | `fclose` |
| `managed_popen(name, cmd, mode)` | `popen` | `pclose` |
| `managed_creat(name, path, mode)` | `creat` | `close` |
| `managed_dup(name, oldfd)` | `dup` | `close` |
| `managed_dup2(name, oldfd, newfd)` | `dup2` | `close` |
| `managed_socket(name, dom, type, proto)` | `socket` | `close` |
| `managed_accept(name, sockfd, addr, len)` | `accept` | `close` |
| `managed_mkstemp(name, tmpl)` | `mkstemp` | `close` |
| `managed_shm(name, shm_name, oflag, mode)` | `shm_open` | `close` |
| `managed_dlopen(name, path, flags)` | `dlopen` | `dlclose` |
| `managed_locale(name, mask, loc, base)` | `newlocale` | `freelocale` |

### Linux (when `__linux__` is defined)

| Macro | Acquires | Releases |
|-------|----------|----------|
| `managed_epoll(name, size)` | `epoll_create` | `close` |
| `managed_epoll1(name, flags)` | `epoll_create1` | `close` |
| `managed_eventfd(name, initval, flags)` | `eventfd` | `close` |
| `managed_timerfd(name, clockid, flags)` | `timerfd_create` | `close` |
| `managed_signalfd(name, fd, mask, flags)` | `signalfd` | `close` |
| `managed_inotify(name)` | `inotify_init` | `close` |
| `managed_inotify1(name, flags)` | `inotify_init1` | `close` |

### Custom types

Anything not covered above works with the generic `managed()` or `defer()`:

```c
// mmap/munmap -- munmap needs ptr AND length
void *p = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
defer(munmap(p, len)) {
    // use p
}

// pipe -- produces two FDs
int pfd[2];
pipe(pfd);
defer(close(pfd[0]))
defer(close(pfd[1])) {
    // use pfd
}
```

## Limitations

- **`return` inside a block SKIPS cleanup.** This is inherent to portable C -- there is no way to intercept `return` without compiler extensions. Keep blocks short and avoid returning from inside them.
- **`break` exits the block** and cleanup runs normally. This is by design.
- **Not thread-safe by itself.** The macros manage scope, not synchronization. Use `RAII_GUARD` with a mutex for thread safety.

## How It Works

Three nested `for` loops. No magic.

```c
managed(type, name, init, dtor)
```

expands to:

```c
for (int _done = 0; !_done; _done = 1)        // loop 1: sentinel
for (type name = init; !_done;                  // loop 2: resource + cleanup
     ((name) ? (void)(dtor)(name) : (void)0), _done = 1)
for (; !_done; _done = 1)                      // loop 3: user body
```

- Loop 3 is the user's `{ }` block. `break` exits loop 3.
- Loop 2's increment runs cleanup after loop 3 finishes (or breaks).
- Loop 1 ensures the whole construct runs exactly once.
- The `(void)` cast silences `-Werror -pedantic` when the destructor returns non-void.

## Building

```sh
make test               # build and run all 36 tests
make example            # build and run examples
make clean              # remove build artifacts

CC=clang make test      # test with clang
CC=gcc make test        # test with gcc
```

Compiler flags: `-Wall -Wextra -Werror -pedantic -std=c99`

## License
This project is licensed under the DBaJ-GPL license. See the [LICENSE](LICENCE) file for details.
