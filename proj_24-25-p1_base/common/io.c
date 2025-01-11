
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "constants.h"

int read_all(int fd, void *buffer, size_t size, int *intr) {
  printf("reading\n");
  if (intr != NULL && *intr) {
    printf("mau\n");
    return -1;
  }
  printf("aqui\n");
  size_t bytes_read = 0;
  printf("Size: %ld\n", size);
  while (bytes_read < size) {
    printf("bytes:%ld\n",bytes_read);
    printf("fd:%d\n",fd);
    ssize_t result = read(fd, buffer + bytes_read, size - bytes_read);
    printf("result:%ld\n",result);
    if (result == -1) {
      if (errno == EINTR) {
        if (intr != NULL) {
          *intr = 1;
          if (bytes_read == 0) {
            return -1;
          }
        }
        continue;
      }
      perror("Failed to read from pipe");
      return -1;
    } else if (result == 0) {
      return 0;
    }
    bytes_read += (size_t)result;
    printf("final do while\n");
  }
  return 1;
}

int read_string(int fd, char *str) {
  ssize_t bytes_read = 0;
  char ch;
  while (bytes_read < MAX_STRING_SIZE - 1) {
    if (read(fd, &ch, 1) != 1) {
      return -1;
    }
    if (ch == '\0' || ch == '\n') {
      break;
    }
    str[bytes_read++] = ch;
  }
  str[bytes_read] = '\0';
  return (int)bytes_read;
}

int write_all(int fd, const void *buffer, size_t size) {
  printf("dentro do write\n");
  size_t bytes_written = 0;
  printf("Size: %ld\n", size);
  while (bytes_written < size) {
    printf("dentro do write\n");
    ssize_t result = write(fd, buffer + bytes_written, size - bytes_written);
    if (result == -1) {
      if (errno == EINTR) {
        // error for broken PIPE (error associated with writting to the closed
        // PIPE)
        continue;
      }
      perror("Failed to write to pipe");
      return -1;
    }
    bytes_written += (size_t)result;
  }
  printf("fim do write\n");
  return 1;
}

static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

void delay(unsigned int time_ms) {
  struct timespec delay = delay_to_timespec(time_ms);
  nanosleep(&delay, NULL);
}
