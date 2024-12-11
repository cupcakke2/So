#ifndef KVS_OPERATIONS_H
#define KVS_OPERATIONS_H

#include <stddef.h>

/// Initializes the KVS state.
/// @return 0 if the KVS state was initialized successfully, 1 otherwise.
int kvs_init();

/// Destroys the KVS state.
/// @return 0 if the KVS state was terminated successfully, 1 otherwise.
int kvs_terminate();

/// Writes a key value pair to the KVS. If key already exists it is updated.
/// @param num_pairs Number of pairs being written.
/// @param keys Array of keys' strings.
/// @param values Array of values' strings.
/// @return 0 if the pairs were written successfully, 1 otherwise.
int kvs_write(size_t num_pairs, char keys[][MAX_STRING_SIZE], char values[][MAX_STRING_SIZE]);

/// Reads values from the KVS.
/// @param fd2 File descriptor to be written in
/// @param num_pairs Number of pairs to read.
/// @param keys Array of keys' strings.
/// @param fd File descriptor to write the (successful) output.
/// @return 0 if the key reading, 1 otherwise.
int kvs_read(int fd2, size_t num_pairs, char keys[][MAX_STRING_SIZE]);

/// Deletes key value pairs from the KVS.
/// @param fd2 File descriptor to be written in
/// @param num_pairs Number of pairs to read.
/// @param keys Array of keys' strings.
/// @return 0 if the pairs were deleted successfully, 1 otherwise.
int kvs_delete(int fd2, size_t num_pairs, char keys[][MAX_STRING_SIZE]);

/// Writes the state of the KVS.
/// @param fd2 File descriptor to write the output.
void kvs_show(int fd2);

/// Creates a backup of the KVS state and stores it in the correspondent
/// backup file
/// @param fd3 file descriptor to be written in
/// @param pid_counts number of current child processes made
/// @param MAX_BACKUPS maximum number of back_ups
/// @return 0 if the backup was successful, 1 otherwise.
int kvs_backup(int fd3, int pid_counts, int MAX_BACKUPS);

/// Waits for the last backup to be called.
void kvs_wait_backup();

/// Waits for a given amount of time.
/// @param delay_us Delay in milliseconds.
void kvs_wait(unsigned int delay_ms);

#endif  // KVS_OPERATIONS_H