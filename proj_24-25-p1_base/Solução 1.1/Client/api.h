#ifndef CLIENT_API_H
#define CLIENT_API_H

#include <stddef.h>
#include <signal.h>
#include "../common/constants.h"

///Pads pipe_path with '\0' at the end to have 40 characters in total
///@param dest char* where the padded_pipe_path will be written
///@param src unpadded pipe path
void pad_pipe_path(char* dest, const char* src);

//Pads pad_key with '\0' at the end to have 41 characters (at most 40 for the name of the key + '\0')
///@param dest char* where the padded key will be written
///@param src unpadded key
void pad_key(char* dest, const char* src);


/// Connects to a kvs server.
/// @param req_pipe_path Path to the name pipe to be created for requests.
/// @param resp_pipe_path Path to the name pipe to be created for responses.
/// @param reg_pipe_path Path to the name pipe where the server is listening.
/// @param notif_pipe_path Path to the name pipe to be created for notifications.
/// @return 0 if the connection was established successfully, 1 otherwise.
int kvs_connect(char const* req_pipe_path, char const* resp_pipe_path, char const* reg_pipe_path,
                char const* notif_pipe_path);
/// Disconnects from an KVS server.
/// fazer mais tarde
/// @return 0 in case of success, 1 otherwise.
int kvs_disconnect(void);

/// Requests a subscription for a key
/// @param key Key to be subscribed
/// @return 1 if the key was subscribed successfully (key existing), 0 otherwise.

int kvs_subscribe(const char* key);

/// Remove a subscription for a key
/// @param key Key to be unsubscribed
/// @return 0 if the key was unsubscribed successfully  (subscription existed and was removed), 1 otherwise.

int kvs_unsubscribe(const char* key);
 
#endif  // CLIENT_API_H
