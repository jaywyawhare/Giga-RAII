/**
 * @file raii.h
 * @brief RAII resource management for C via __attribute__((cleanup)).
 *
 * GCC/Clang only. Cleanup runs on every scope exit (return, break, goto, or
 * falling off the end), so there are no block macros and no return-leak
 * footgun. Declare a variable and it is released when it goes out of scope.
 *
 * @code
 *   autofree char      *buf = malloc(1024);
 *   autoclose_file FILE *f   = fopen(p, "r");
 *   autoclose_fd   int   fd  = open(p, 0);
 * @endcode
 */
#ifndef GIGA_RAII_H
#define GIGA_RAII_H

#if !defined(__GNUC__) && !defined(__clang__)
#  error "Giga-RAII requires GCC or Clang (uses __attribute__((cleanup)))."
#endif

#include <stdlib.h>
#include <stdio.h>

/**
 * @brief Attach a cleanup function to a variable; it runs at scope exit.
 * @param fn  A `void fn(T *)` that receives the variable's address.
 */
#define RAII_CLEANUP(fn) __attribute__((cleanup(fn)))

/**
 * @brief Define a NULL-safe cleanup function for a pointer resource type.
 *
 * Generates `static inline void fnname(type *)` that calls @p dtor on the
 * pointed-to value when it is non-NULL. Use the generated name with
 * RAII_CLEANUP() for one-line scoped management of a custom type. Must be
 * used at file scope.
 *
 * @code
 *   RAII_CLEANUP_FN(close_db, db_t*, db_close);
 *   RAII_CLEANUP(close_db) db_t *h = db_open("x");
 * @endcode
 *
 * @param fnname  Name of the generated cleanup function.
 * @param type    Pointer resource type (e.g. `FILE*`, `db_t*`).
 * @param dtor    Destructor taking one @p type argument.
 */
#define RAII_CLEANUP_FN(fnname, type, dtor)          \
    static inline void fnname(type *_raii_p)         \
    {                                                \
        if (*_raii_p)                                \
            (dtor)(*_raii_p);                        \
    }                                                \
    struct raii_force_semi_##fnname

/** @cond INTERNAL */
static inline void raii_free_(void *p) { free(*(void **)p); }
RAII_CLEANUP_FN(raii_fclose_, FILE*, fclose);
/** @endcond */

/**
 * @brief Pointer that is free()d at scope exit. NULL-safe.
 * @code autofree char *s = malloc(n); @endcode
 */
#define autofree       RAII_CLEANUP(raii_free_)

/**
 * @brief FILE* that is fclose()d at scope exit. NULL-safe.
 * @code autoclose_file FILE *f = fopen(path, "r"); @endcode
 */
#define autoclose_file RAII_CLEANUP(raii_fclose_)

/**
 * @brief Typed single-object allocation freed at scope exit: `T *name`.
 * @param T     Object type.
 * @param name  Variable name (type `T*`).
 */
#define managed_new(T, name) \
    autofree T *name = malloc(sizeof(T))

/**
 * @brief Typed array allocation freed at scope exit: `T *name`.
 * @param T     Element type.
 * @param name  Variable name (type `T*`).
 * @param n     Number of elements.
 */
#define managed_array(T, name, n) \
    autofree T *name = malloc(sizeof(T) * (size_t)(n))

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L

#include <dirent.h>
#include <dlfcn.h>
#include <locale.h>
#include <unistd.h>

/** @cond INTERNAL */
static inline void raii_closefd_(int *fd) { if (*fd >= 0) close(*fd); }
RAII_CLEANUP_FN(raii_closedir_, DIR*, closedir);
RAII_CLEANUP_FN(raii_pclose_, FILE*, pclose);
RAII_CLEANUP_FN(raii_dlclose_, void*, dlclose);
RAII_CLEANUP_FN(raii_freelocale_, locale_t, freelocale);
/** @endcond */

/**
 * @brief int file descriptor close()d at scope exit (if >= 0).
 *
 * Covers every fd-returning call: open, creat, dup, socket, accept, mkstemp,
 * shm_open, epoll_create, eventfd, timerfd_create, signalfd, inotify_init.
 * @code autoclose_fd int fd = socket(AF_INET, SOCK_STREAM, 0); @endcode
 */
#define autoclose_fd     RAII_CLEANUP(raii_closefd_)

/** @brief DIR* closedir()d at scope exit. NULL-safe. */
#define autoclose_dir    RAII_CLEANUP(raii_closedir_)

/** @brief popen() stream pclose()d at scope exit. NULL-safe. */
#define autoclose_pipe   RAII_CLEANUP(raii_pclose_)

/** @brief dlopen() handle dlclose()d at scope exit. NULL-safe. */
#define autoclose_dl     RAII_CLEANUP(raii_dlclose_)

/** @brief newlocale() handle freelocale()d at scope exit. NULL-safe. */
#define autoclose_locale RAII_CLEANUP(raii_freelocale_)

#endif /* POSIX */

#endif /* GIGA_RAII_H */
