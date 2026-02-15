/**
 * @file raii.h
 * @brief Portable RAII resource management for C (C99+).
 *
 * @warning `return` from inside a managed/defer block SKIPS cleanup.
 *          `break` exits the block and cleanup runs normally.
 *
 */
#ifndef GIGA_RAII_H
#define GIGA_RAII_H

#include <stdlib.h>
#include <stdio.h>

/** @cond INTERNAL */
#define RAII_CAT_(a, b)  a##b
#define RAII_CAT(a, b)   RAII_CAT_(a, b)
#define RAII_DONE_(name)  RAII_CAT(_raii_done_, name)

#ifdef __COUNTER__
#  define DEFER_ID_ RAII_CAT(_df_, __COUNTER__)
#else
#  define DEFER_ID_ RAII_CAT(_df_, __LINE__)
#endif
/** @endcond */

/**
 * @brief Scoped resource management. Calls @p dtor(name) at block exit.
 *
 * NULL-safe: if @p init evaluates to NULL, @p dtor is not called.
 *
 * @param type  Resource type (e.g. `void*`, `FILE*`).
 * @param name  Variable name bound in the block scope.
 * @param init  Initializer expression (e.g. `malloc(128)`).
 * @param dtor  Destructor function (e.g. `free`).
 */
#define managed(type, name, init, dtor)                                      \
    for (int RAII_DONE_(name) = 0; !RAII_DONE_(name); RAII_DONE_(name) = 1)\
    for (type name = (init); !RAII_DONE_(name);                             \
         ((name) ? (void)(dtor)(name) : (void)0), RAII_DONE_(name) = 1)    \
    for (; !RAII_DONE_(name); RAII_DONE_(name) = 1)

/**
 * @brief Scoped FILE* via fopen()/fclose().
 * @param name  Variable name bound in the block scope.
 * @param path  File path.
 * @param mode  fopen mode string.
 */
#define managed_file(name, path, mode) \
    managed(FILE*, name, fopen((path), (mode)), fclose)

/**
 * @brief Scoped file descriptor via close(). Uses -1 as invalid sentinel.
 * @param name       Variable name bound in the block scope.
 * @param open_expr  Expression returning an fd (e.g. `open("f", O_RDONLY)`).
 */
#define managed_fd(name, open_expr)                                         \
    for (int RAII_DONE_(name) = 0; !RAII_DONE_(name); RAII_DONE_(name) = 1)\
    for (int name = (open_expr); !RAII_DONE_(name);                        \
         (raii_close_fd(name), RAII_DONE_(name) = 1))                      \
    for (; !RAII_DONE_(name); RAII_DONE_(name) = 1)

/**
 * @brief Alias for managed(). Provided for readability with custom types.
 * @param type  Resource type.
 * @param name  Variable name bound in the block scope.
 * @param init  Initializer expression.
 * @param dtor  Destructor function.
 */
#define managed_with(type, name, init, dtor) \
    managed(type, name, init, dtor)

/**
 * @brief Deferred cleanup. Runs @p expr at block exit. Break-safe.
 *
 * Stack multiple for LIFO order: the innermost defer runs first.
 *
 * @param expr  Expression to evaluate at block exit.
 */
#define defer(expr) defer_(DEFER_ID_, expr)

/** @cond INTERNAL */
#define defer_(uid, expr)                                                   \
    for (int uid = 0; !uid; (uid = 1, (void)(expr)))                       \
    for (; !uid; uid = 1)
/** @endcond */

/**
 * @brief Scoped lock/unlock guard on an existing variable.
 * @param var      Lock variable (passed by address to acquire/release).
 * @param acquire  Lock function (e.g. `pthread_mutex_lock`).
 * @param release  Unlock function (e.g. `pthread_mutex_unlock`).
 */
#define RAII_GUARD(var, acquire, release)                                    \
    defer((void)(release)(&(var)))                                          \
    for (int RAII_DONE_(var) = ((void)(acquire)(&(var)), 0);                \
         !RAII_DONE_(var); RAII_DONE_(var) = 1)

/** @name C99 Convenience Macros
 *  @{ */

/**
 * @brief Scoped malloc()/free().
 * @param name  Variable name (void*).
 * @param size  Allocation size in bytes.
 */
#define managed_malloc(name, size) \
    managed(void*, name, malloc(size), free)

/**
 * @brief Scoped calloc()/free().
 * @param name   Variable name (void*).
 * @param nmemb  Number of elements.
 * @param size   Element size in bytes.
 */
#define managed_calloc(name, nmemb, size) \
    managed(void*, name, calloc((nmemb), (size)), free)

/**
 * @brief Scoped tmpfile()/fclose().
 * @param name  Variable name (FILE*).
 */
#define managed_tmpfile(name) \
    managed(FILE*, name, tmpfile(), fclose)

/** @} */

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L

/** @name C11 Convenience Macros
 *  @{ */

/**
 * @brief Scoped aligned_alloc()/free().
 * @param name   Variable name (void*).
 * @param align  Alignment in bytes (must be power of 2).
 * @param size   Allocation size (must be multiple of @p align).
 */
#define managed_aligned_alloc(name, align, size) \
    managed(void*, name, aligned_alloc((align), (size)), free)

/** @} */

#endif /* C11 */

#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L

#include <dirent.h>
#include <dlfcn.h>
#include <locale.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

/** @name POSIX Convenience Macros
 *  @{ */

/**
 * @brief Scoped strdup()/free().
 * @param name  Variable name (char*).
 * @param s     Source string.
 */
#define managed_strdup(name, s) \
    managed(char*, name, strdup(s), free)

/**
 * @brief Scoped strndup()/free().
 * @param name  Variable name (char*).
 * @param s     Source string.
 * @param n     Maximum bytes to copy.
 */
#define managed_strndup(name, s, n) \
    managed(char*, name, strndup((s), (n)), free)

/**
 * @brief Scoped opendir()/closedir().
 * @param name  Variable name (DIR*).
 * @param path  Directory path.
 */
#define managed_dir(name, path) \
    managed(DIR*, name, opendir(path), closedir)

/**
 * @brief Scoped fdopendir()/closedir().
 * @param name  Variable name (DIR*).
 * @param fd    Open file descriptor for a directory.
 */
#define managed_fdopendir(name, fd) \
    managed(DIR*, name, fdopendir(fd), closedir)

/**
 * @brief Scoped fdopen()/fclose().
 * @param name  Variable name (FILE*).
 * @param fd    Open file descriptor.
 * @param mode  fopen mode string.
 */
#define managed_fdopen(name, fd, mode) \
    managed(FILE*, name, fdopen((fd), (mode)), fclose)

/**
 * @brief Scoped popen()/pclose().
 * @param name  Variable name (FILE*).
 * @param cmd   Shell command string.
 * @param mode  "r" or "w".
 */
#define managed_popen(name, cmd, mode) \
    managed(FILE*, name, popen((cmd), (mode)), pclose)

/**
 * @brief Scoped creat()/close().
 * @param name  Variable name (int fd).
 * @param path  File path.
 * @param mode  File permission bits.
 */
#define managed_creat(name, path, mode) \
    managed_fd(name, creat((path), (mode)))

/**
 * @brief Scoped dup()/close().
 * @param name   Variable name (int fd).
 * @param oldfd  File descriptor to duplicate.
 */
#define managed_dup(name, oldfd) \
    managed_fd(name, dup(oldfd))

/**
 * @brief Scoped dup2()/close().
 * @param name   Variable name (int fd).
 * @param oldfd  Source file descriptor.
 * @param newfd  Target file descriptor number.
 */
#define managed_dup2(name, oldfd, newfd) \
    managed_fd(name, dup2((oldfd), (newfd)))

/**
 * @brief Scoped socket()/close().
 * @param name   Variable name (int fd).
 * @param dom    Address family (e.g. AF_INET).
 * @param type   Socket type (e.g. SOCK_STREAM).
 * @param proto  Protocol (usually 0).
 */
#define managed_socket(name, dom, type, proto) \
    managed_fd(name, socket((dom), (type), (proto)))

/**
 * @brief Scoped accept()/close().
 * @param name    Variable name (int fd).
 * @param sockfd  Listening socket fd.
 * @param addr    Pointer to sockaddr (or NULL).
 * @param len     Pointer to socklen_t (or NULL).
 */
#define managed_accept(name, sockfd, addr, len) \
    managed_fd(name, accept((sockfd), (addr), (len)))

/**
 * @brief Scoped mkstemp()/close().
 * @param name  Variable name (int fd).
 * @param tmpl  Template path ending in "XXXXXX" (modified in-place).
 */
#define managed_mkstemp(name, tmpl) \
    managed_fd(name, mkstemp(tmpl))

/**
 * @brief Scoped shm_open()/close().
 * @param name      Variable name (int fd).
 * @param shm_name  Shared memory object name.
 * @param oflag     Open flags.
 * @param mode      Permission bits.
 */
#define managed_shm(name, shm_name, oflag, mode) \
    managed_fd(name, shm_open((shm_name), (oflag), (mode)))

/**
 * @brief Scoped dlopen()/dlclose().
 * @param name   Variable name (void*).
 * @param path   Library path (or NULL).
 * @param flags  dlopen flags (e.g. RTLD_LAZY).
 */
#define managed_dlopen(name, path, flags) \
    managed(void*, name, dlopen((path), (flags)), dlclose)

/**
 * @brief Scoped newlocale()/freelocale().
 * @param name  Variable name (locale_t).
 * @param mask  Category mask (e.g. LC_ALL_MASK).
 * @param loc   Locale name string.
 * @param base  Base locale (or 0).
 */
#define managed_locale(name, mask, loc, base) \
    managed(locale_t, name, newlocale((mask), (loc), (base)), freelocale)

/** @} */

#endif /* POSIX */

#if defined(__linux__)

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>
#include <sys/inotify.h>

/** @name Linux Convenience Macros
 *  @{ */

/**
 * @brief Scoped epoll_create()/close().
 * @param name  Variable name (int fd).
 * @param size  Hint for number of fds (ignored since Linux 2.6.8).
 */
#define managed_epoll(name, size) \
    managed_fd(name, epoll_create(size))

/**
 * @brief Scoped epoll_create1()/close().
 * @param name   Variable name (int fd).
 * @param flags  EPOLL_CLOEXEC or 0.
 */
#define managed_epoll1(name, flags) \
    managed_fd(name, epoll_create1(flags))

/**
 * @brief Scoped eventfd()/close().
 * @param name     Variable name (int fd).
 * @param initval  Initial counter value.
 * @param flags    EFD_CLOEXEC, EFD_NONBLOCK, EFD_SEMAPHORE, or 0.
 */
#define managed_eventfd(name, initval, flags) \
    managed_fd(name, eventfd((initval), (flags)))

/**
 * @brief Scoped timerfd_create()/close().
 * @param name     Variable name (int fd).
 * @param clockid  Clock id (e.g. CLOCK_MONOTONIC).
 * @param flags    TFD_CLOEXEC, TFD_NONBLOCK, or 0.
 */
#define managed_timerfd(name, clockid, flags) \
    managed_fd(name, timerfd_create((clockid), (flags)))

/**
 * @brief Scoped signalfd()/close().
 * @param name   Variable name (int fd).
 * @param fd     Existing signalfd to update, or -1 for new.
 * @param mask   Pointer to sigset_t.
 * @param flags  SFD_CLOEXEC, SFD_NONBLOCK, or 0.
 */
#define managed_signalfd(name, fd, mask, flags) \
    managed_fd(name, signalfd((fd), (mask), (flags)))

/**
 * @brief Scoped inotify_init()/close().
 * @param name  Variable name (int fd).
 */
#define managed_inotify(name) \
    managed_fd(name, inotify_init())

/**
 * @brief Scoped inotify_init1()/close().
 * @param name   Variable name (int fd).
 * @param flags  IN_CLOEXEC, IN_NONBLOCK, or 0.
 */
#define managed_inotify1(name, flags) \
    managed_fd(name, inotify_init1(flags))

/** @} */

#endif /* __linux__ */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Close a file descriptor if valid (fd >= 0).
 * @param fd  File descriptor to close.
 */
void raii_close_fd(int fd);

#ifdef __cplusplus
}
#endif

#endif /* GIGA_RAII_H */
