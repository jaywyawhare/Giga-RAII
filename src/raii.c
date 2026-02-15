/**
 * @file raii.c
 * @brief Giga-RAII utility functions.
 */
#include "raii.h"
#include <unistd.h>

void raii_close_fd(int fd)
{
    if (fd >= 0)
        close(fd);
}
