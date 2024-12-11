#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "kvs.h"
#include "constants.h"

static struct HashTable* kvs_table = NULL;


/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

//Comparing fuction used in qsort to order the output of kvs_read
int compare(const void *a, const void *b) {
    const char *strA = *(const char **)a;
    const char *strB = *(const char **)b;
    return strcmp(strA, strB);
}

char *sort_parentheses(const char *input_str) {
    static char result[MAX_OUT_BUFFER_SIZE]; 
    char *pairs[MAX_OUT_BUFFER_SIZE];         
    char temp[MAX_OUT_BUFFER_SIZE];          
    size_t count = 0;

    strncpy(temp, input_str + 1, strlen(input_str) - 2); 
    temp[strlen(input_str) - 2] = '\0';

    char *token = strtok(temp, "(");
    while (token != NULL) {
        pairs[count] = token;
        char *end = strchr(pairs[count], ')');
        if (end) *end = '\0';
        count++;
        token = strtok(NULL, "(");
    }

    // Sort the pairs using qsort function defined above
    qsort(pairs, count, sizeof(char *), compare);

    char *res_ptr = result;
    *res_ptr++ = '[';
    for (size_t i = 0; i < count; i++) {
        res_ptr += sprintf(res_ptr, "(%s)", pairs[i]);
    }
    *res_ptr++ = ']';
    *res_ptr = '\0';

    return result;
}

int kvs_init() {
  if (kvs_table != NULL) {
    fprintf(stderr, "KVS state has already been initialized\n");
    return 1;
  }
  kvs_table = create_hash_table();
  pthread_rwlock_init(&kvs_table->tree_lock, NULL);
  for (int i = 0; i < TABLE_SIZE; i++) {
    pthread_rwlock_init(&kvs_table->rwlocks[i], NULL);
  }
  return kvs_table == NULL;
}

int kvs_terminate() {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  for (int i = 0; i < TABLE_SIZE; i++) {
    pthread_rwlock_destroy(&kvs_table->rwlocks[i]);
  }
  pthread_rwlock_destroy(&kvs_table->tree_lock);

  free_table(kvs_table);
  return 0;
}

int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]) {
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  for (size_t i = 0; i < num_pairs; i++) {

    int entry = hash(keys[i]) % TABLE_SIZE;

    pthread_rwlock_wrlock(&kvs_table->rwlocks[entry]);
    if (write_pair(kvs_table, keys[i], values[i]) != 0) {
      fprintf(stderr, "Failed to write keypair (%s,%s)\n", keys[i], values[i]);
    }

    pthread_rwlock_unlock(&kvs_table->rwlocks[entry]);
  }

  return 0;
}



int kvs_read(int fd2, size_t num_pairs, char keys[][MAX_STRING_SIZE]) {
  char buffer[MAX_OUT_BUFFER_SIZE] = ""; 
  strcat(buffer, "[");
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }

  

  for (size_t i = 0; i < num_pairs; i++) {

    int entry = hash(keys[i]) % TABLE_SIZE;
    pthread_rwlock_rdlock(&kvs_table->rwlocks[entry]);
    
    

    char* result = read_pair(kvs_table, keys[i]);
    if (result == NULL) {
        strcat(buffer, "(");
        strcat(buffer, keys[i]);
        strcat(buffer, ",KVSERROR)");
    } else {
        strcat(buffer, "(");
        strcat(buffer, keys[i]);
        strcat(buffer, ",");
        strcat(buffer, result);
        strcat(buffer, ")");
    }

   // Add newline for readability

    // Write only the valid part of the buffer to the file
    
    free(result);
    pthread_rwlock_unlock(&kvs_table->rwlocks[entry]); 
  }

  strcat(buffer, "]"); 
  char *sorted_buffer = sort_parentheses(buffer);
  strcat(sorted_buffer, "\n");
  write(fd2, sorted_buffer, strlen(sorted_buffer));
  
  return 0;
}

int kvs_delete(int fd2, size_t num_pairs, char keys[][MAX_STRING_SIZE]) {
  char buffer[MAX_OUT_BUFFER_SIZE] = ""; // Reinitialize buffer for each iteration
  if (kvs_table == NULL) {
    fprintf(stderr, "KVS state must be initialized\n");
    return 1;
  }
  int aux = 0;

  for (size_t i = 0; i < num_pairs; i++) {

    int entry = hash(keys[i]) % TABLE_SIZE;
    pthread_rwlock_wrlock(&kvs_table->rwlocks[entry]);
    if (delete_pair(kvs_table, keys[i]) != 0) {
      if (!aux) {
        strcat(buffer,"[");
        aux = 1;
      }
      strcat(buffer,"(");
      strcat(buffer,keys[i]);
      strcat(buffer,",KVSMISSING)");
    }
    
    pthread_rwlock_unlock(&kvs_table->rwlocks[entry]);
  }
  if (aux) {
    strcat(buffer,"]\n");
  }
  write(fd2, buffer, strlen(buffer));
  return 0;
}

void kvs_show(int fd2) {
  char buffer[MAX_OUT_BUFFER_SIZE] = ""; // Reinitialize buffer for each iteration

  pthread_rwlock_rdlock(&kvs_table->tree_lock);
  for (int i = 0; i < TABLE_SIZE; i++) {
    KeyNode *keyNode = kvs_table->table[i];
    while (keyNode != NULL) {
      strcat(buffer,"(");
      strcat(buffer,keyNode->key);
      strcat(buffer,", ");
      strcat(buffer,keyNode->value);
      strcat(buffer,")\n");
      keyNode = keyNode->next; // Move to the next node
    }
  }
  write(fd2, buffer, strlen(buffer));
  pthread_rwlock_unlock(&kvs_table->tree_lock);
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
    _exit(0);
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