#ifndef COMMON_IO_H
#define COMMON_IO_H

#include <stddef.h>

 

int read_string(int fd, char *str);

/// Writes a given number of bytes to a file descriptor. Will block until all
/// bytes are written, or fail if not all bytes could be written.
/// @param fd File descriptor to write to.
/// @param buffer Buffer to write from.
/// @param size Number of bytes to write.
/// @return On success, returns 1, on error, returns -1
int write_all(int fd, const void *buffer, size_t size);

void delay(unsigned int time_ms);

#endif // COMMON_IO_H
