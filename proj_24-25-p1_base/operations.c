#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>

#include "kvs.h"
#include "constants.h"

static struct HashTable* kvs_table = NULL;


/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

int kvs_init() {
  if (kvs_table != NULL) {
    fprintf(stderr, "KVS state has already been initialized\n");
    return 1;
  }

  kvs_table = create_hash_table();
  return kvs_table == NULL;
}

int kvs_terminate() {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  free_table(kvs_table);
  return 0;
}

int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE],pthread_rwlock_t* rwlock) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  pthread_rwlock_wrlock(rwlock);
  for (size_t i = 0; i < num_pairs; i++) {
    if (write_pair(kvs_table, keys[i], values[i]) != 0) {
      fprintf(stderr, "Failed to write keypair (%s,%s)\n", keys[i], values[i]);
    }
  }
  pthread_rwlock_unlock(rwlock);
  return 0;
}

int kvs_read(int fd2, size_t num_pairs, char keys[][MAX_STRING_SIZE],pthread_rwlock_t* rwlock) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  pthread_rwlock_rdlock(rwlock);
  for (size_t i = 0; i < num_pairs; i++) {
    char buffer[MAX_OUT_BUFFER_SIZE] = ""; // Reinitialize buffer for each iteration
    strcat(buffer, "[");

    char* result = read_pair(kvs_table, keys[i]);
    if (result == NULL) {
        strcat(buffer, "(");
        strcat(buffer, keys[i]);
        strcat(buffer, ",KVSERROR)]");
    } else {
        strcat(buffer, "(");
        strcat(buffer, keys[i]);
        strcat(buffer, ",");
        strcat(buffer, result);
        strcat(buffer, ")]");
    }

    strcat(buffer, "\n"); // Add newline for readability

    // Write only the valid part of the buffer to the file
    write(fd2, buffer, strlen(buffer));
    free(result);
  }
  pthread_rwlock_unlock(rwlock);
  return 0;
}

int kvs_delete(int fd2, size_t num_pairs, char keys[][MAX_STRING_SIZE],pthread_rwlock_t* rwlock) {
  char buffer[MAX_OUT_BUFFER_SIZE] = ""; // Reinitialize buffer for each iteration
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }
  int aux = 0;

  pthread_rwlock_wrlock(rwlock);
  for (size_t i = 0; i < num_pairs; i++) {
    if (delete_pair(kvs_table, keys[i]) != 0) {
      if (!aux) {
        strcat(buffer,"[");
        aux = 1;
      }
      strcat(buffer,"(");
      strcat(buffer,keys[i]);
      strcat(buffer,",KVSMISSING)");
    }
  }
  if (aux) {
    strcat(buffer,"]\n");
  }
  write(fd2, buffer, strlen(buffer));
  pthread_rwlock_unlock(rwlock);
  return 0;
}

void kvs_show(int fd2) {
  char buffer[MAX_OUT_BUFFER_SIZE] = ""; // Reinitialize buffer for each iteration
  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    while (keyNode != NULL) {
      strcat(buffer,"(");
      strcat(buffer,keyNode->key);
      strcat(buffer,",");
      strcat(buffer,keyNode->value);
      strcat(buffer,")\n");
      keyNode = keyNode->next; // Move to the next node
    }
  }
  write(fd2, buffer, strlen(buffer));
}

int kvs_backup(int fd3, int pid_counts, int MAX_BACKUPS) {

  pid_t pid;
  pid = fork ();
  if (pid == -1){
    fprintf(stderr, "Failed to fork.\n");
    return 1;
  }
  if (pid == 0) {

    kvs_show(fd3);  
    exit(0);
  } else {
  if(pid_counts >= MAX_BACKUPS)
    wait(NULL);
  }

  return 0;
}

void kvs_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}