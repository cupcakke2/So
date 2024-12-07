#ifndef STRUCT_H
#define STRUCT_H

#include <pthread.h>
#include "constants.h"

typedef struct {
    int fd;
    pthread_rwlock_t rwlock;
} fd_with_mutex;

typedef struct {
    size_t num_pairs;
    char keys [256][MAX_STRING_SIZE];
    char values [256][MAX_STRING_SIZE];
    fd_with_mutex file;
} write_args_t;

#endif 