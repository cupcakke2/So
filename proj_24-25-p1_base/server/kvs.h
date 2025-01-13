#ifndef KEY_VALUE_STORE_H
#define KEY_VALUE_STORE_H
#define TABLE_SIZE 26

#include <pthread.h>
#include <stddef.h>
#include "../common/constants.h"

typedef struct KeyNode {
  char *key;
  char *value;
  struct KeyNode *next;
} KeyNode;

typedef struct HashTable {
  KeyNode *table[TABLE_SIZE];
  pthread_rwlock_t tablelock;
} HashTable;

typedef struct {  
  int key_count;
  int client_id;
  int client_index;
  char client_keys[MAX_KEY_SIZE][MAX_NUMBER_SUB];
  char req_pipe_path[MAX_PIPE_PATH_LENGTH];
  char resp_pipe_path[MAX_PIPE_PATH_LENGTH];
  char notif_pipe_path[MAX_PIPE_PATH_LENGTH];
} Client;


/// Creates a new KVS hash table.
/// @return Newly created hash table, NULL on failure
struct HashTable *create_hash_table();

extern char global_keys[MAX_KEY_SIZE][MAX_NUMBER_SUB];
extern char notif_pipe_path[MAX_PIPE_PATH_LENGTH];

int hash(const char *key);

//Pads pad_key with '\0' at the end to have 41 characters (at most 40 for the name of the key + '\0')
///@param dest char* where the padded key will be written
///@param src unpadded key
void pad_key_or_value(char* dest, const char* src);

// Writes a key value pair in the hash table.
// @param ht The hash table.
// @param key The key.
// @param value The value.
// @return 0 if successful.
int write_pair(HashTable *ht, const char *key, const char *value);

// Reads the value of a given key.
// @param ht The hash table.
// @param key The key.
// return the value if found, NULL otherwise.
char *read_pair(HashTable *ht, const char *key);

/// Deletes a pair from the table.
/// @param ht Hash table to read from.
/// @param key Key of the pair to be deleted.
/// @return 0 if the node was deleted successfully, 1 otherwise.
int delete_pair(HashTable *ht, const char *key);

/// Frees the hashtable.
/// @param ht Hash table to be deleted.
void free_table(HashTable *ht);

#endif // KVS_H