/**
 * @file raii.h
 * @brief Portable RAII resource management for C, with a return-safe layer.
 *
 * Two tiers:
 *   - Portable block macros (managed, managed_ok, managed_file, managed_fd,
 *     managed_new, managed_array, defer, RAII_GUARD). Pure C99, so they build
 *     on any compiler including MSVC. Cleanup runs at block exit; a `return`
 *     out of the block skips it, so keep blocks short.
 *   - Return-safe declarations (autofree, autoclose_*, RAII_CLEANUP,
 *     RAII_CLEANUP_FN). Available on GCC/Clang only (guarded by
 *     RAII_HAS_CLEANUP); cleanup runs on every scope exit, including `return`.
 *
 * @code
 *   managed(char*, buf, malloc(256), free) { ... }   // every compiler
 *   autofree char *buf = malloc(256);                // GCC/Clang, return-safe
 * @endcode
 */
#ifndef GIGA_RAII_H
#define GIGA_RAII_H

#include <stdlib.h>
#include <stdio.h>

#if defined(_WIN32)
#  include <io.h>
#  define RAII_CLOSE_(fd) _close(fd)
#else
#  include <unistd.h>
#  define RAII_CLOSE_(fd) close(fd)
#endif

/* ====================================================================== */
/*  Portable tier: any C99 compiler, including MSVC.                       */
/* ====================================================================== */

/** @cond INTERNAL */
#define RAII_CAT_(a, b)  a##b
#define RAII_CAT(a, b)   RAII_CAT_(a, b)
#define RAII_DONE_(name) RAII_CAT(_raii_done_, name)

/* __COUNTER__ lets several defers share a source line, but strict-ISO clang
 * diagnoses it as a C2y extension under -pedantic, so fall back to __LINE__. */
#if defined(__COUNTER__) && !(defined(__clang__) && defined(__STRICT_ANSI__))
#  define RAII_DEFER_ID_ RAII_CAT(_raii_df_, __COUNTER__)
#else
#  define RAII_DEFER_ID_ RAII_CAT(_raii_df_, __LINE__)
#endif

#define RAII_DEFER_(uid, expr)                       \
    for (int uid = 0; !uid; (uid = 1, (void)(expr))) \
    for (; !uid; uid = 1)
/** @endcond */

/**
 * @brief Close a file descriptor if valid (fd >= 0). Portable (POSIX/Windows).
 * @param fd  File descriptor to close.
 */
static inline void raii_close_fd(int fd)
{
    if (fd >= 0)
        (void)RAII_CLOSE_(fd);
}

/**
 * @brief Scoped resource; calls @p dtor(name) at block exit. NULL-safe.
 * @param type  Resource type (e.g. `void*`, `FILE*`).
 * @param name  Variable bound in the block scope.
 * @param init  Initializer expression.
 * @param dtor  Destructor function.
 */
#define managed(type, name, init, dtor)                                      \
    for (int RAII_DONE_(name) = 0; !RAII_DONE_(name); RAII_DONE_(name) = 1)  \
    for (type name = (init); !RAII_DONE_(name);                              \
         ((name) ? (void)(dtor)(name) : (void)0), RAII_DONE_(name) = 1)      \
    for (; !RAII_DONE_(name); RAII_DONE_(name) = 1)

/**
 * @brief Like managed(), but the block body runs only when @p init is non-NULL.
 *
 * Removes the `if (!name) break;` guard. Pointer resources only.
 * @param type  Pointer resource type.
 * @param name  Variable bound in the block scope.
 * @param init  Initializer expression.
 * @param dtor  Destructor function.
 */
#define managed_ok(type, name, init, dtor)                                   \
    for (int RAII_DONE_(name) = 0; !RAII_DONE_(name); RAII_DONE_(name) = 1)  \
    for (type name = (init); !RAII_DONE_(name);                              \
         ((name) ? (void)(dtor)(name) : (void)0), RAII_DONE_(name) = 1)      \
    for (; !RAII_DONE_(name) && (name); RAII_DONE_(name) = 1)

/**
 * @brief Scoped typed single-object allocation: `T *name = malloc(sizeof(T))`.
 * @param T     Object type.
 * @param name  Variable bound in the block scope (type `T*`).
 */
#define managed_new(T, name) \
    managed(T*, name, (T*)malloc(sizeof(T)), free)

/**
 * @brief Scoped typed array allocation: `T *name = malloc(sizeof(T) * n)`.
 * @param T     Element type.
 * @param name  Variable bound in the block scope (type `T*`).
 * @param n     Number of elements.
 */
#define managed_array(T, name, n) \
    managed(T*, name, (T*)malloc(sizeof(T) * (size_t)(n)), free)

/**
 * @brief Scoped FILE* via fopen()/fclose().
 * @param name  Variable bound in the block scope.
 * @param path  File path.
 * @param mode  fopen mode string.
 */
#define managed_file(name, path, mode) \
    managed(FILE*, name, fopen((path), (mode)), fclose)

/**
 * @brief Scoped file descriptor via close(). Uses -1 as invalid sentinel.
 * @param name       Variable bound in the block scope.
 * @param open_expr  Expression returning an fd.
 */
#define managed_fd(name, open_expr)                                          \
    for (int RAII_DONE_(name) = 0; !RAII_DONE_(name); RAII_DONE_(name) = 1)  \
    for (int name = (open_expr); !RAII_DONE_(name);                          \
         (raii_close_fd(name), RAII_DONE_(name) = 1))                        \
    for (; !RAII_DONE_(name); RAII_DONE_(name) = 1)

/**
 * @brief Deferred cleanup. Runs @p expr at block exit. Break-safe.
 *        Stack multiple for LIFO order (innermost runs first).
 * @param expr  Expression to evaluate at block exit.
 */
#define defer(expr) RAII_DEFER_(RAII_DEFER_ID_, expr)

/**
 * @brief Scoped lock/unlock guard on an existing variable.
 * @param var      Lock variable (passed by address to acquire/release).
 * @param acquire  Lock function.
 * @param release  Unlock function.
 */
#define RAII_GUARD(var, acquire, release)                                    \
    defer((void)(release)(&(var)))                                           \
    for (int RAII_DONE_(var) = ((void)(acquire)(&(var)), 0);                 \
         !RAII_DONE_(var); RAII_DONE_(var) = 1)

/* ====================================================================== */
/*  Return-safe tier: GCC/Clang only (__attribute__((cleanup))).           */
/* ====================================================================== */

#if defined(__GNUC__) || defined(__clang__)

#define RAII_HAS_CLEANUP 1

/**
 * @brief Attach a cleanup function to a variable; it runs on every scope exit.
 * @param fn  A `void fn(T *)` that receives the variable's address.
 */
#define RAII_CLEANUP(fn) __attribute__((cleanup(fn)))

/**
 * @brief Define a NULL-safe cleanup function for a pointer resource type.
 *
 * Generates `static inline void fnname(type *)` calling @p dtor when the
 * pointed-to value is non-NULL. Use at file scope with RAII_CLEANUP().
 *
 * @code
 *   RAII_CLEANUP_FN(close_db, db_t*, db_close);
 *   RAII_CLEANUP(close_db) db_t *h = db_open("x");
 * @endcode
 *
 * @param fnname  Name of the generated cleanup function.
 * @param type    Pointer resource type.
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

/** @brief Pointer that is free()d at scope exit. NULL-safe. */
#define autofree       RAII_CLEANUP(raii_free_)

/** @brief FILE* that is fclose()d at scope exit. NULL-safe. */
#define autoclose_file RAII_CLEANUP(raii_fclose_)

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L

#include <dirent.h>
#include <dlfcn.h>
#include <locale.h>

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

#endif /* __GNUC__ || __clang__ */

#endif /* GIGA_RAII_H */
