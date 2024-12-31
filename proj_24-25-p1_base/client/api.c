#include "api.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "../common/constants.h"
#include "../common/protocol.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void pad_pipe_path(char* dest, const char* src) {
    strncpy(dest, src,  MAX_PIPE_PATH_LENGTH - 1); // Copy at most max_size - 1 characters
    dest[MAX_PIPE_PATH_LENGTH - 1] = '\0';       // Ensure null termination
    for (size_t i = strlen(src); i < MAX_PIPE_PATH_LENGTH; i++) {
        dest[i] = '\0';              // Fill remaining space with '\0'
    }
}


int kvs_connect(char const* req_pipe_path, char const* resp_pipe_path, char const* reg_pipe_path,
                char const* notif_pipe_path) {

  int freg;              
  char connect_message[MAX_CONNECT_MESSAGE_SIZE] = "";
  char padded_req[MAX_PIPE_PATH_LENGTH];
  char padded_resp[MAX_PIPE_PATH_LENGTH];
  char padded_notif[MAX_PIPE_PATH_LENGTH];

  unlink(req_pipe_path);
  unlink(resp_pipe_path);
  unlink(notif_pipe_path);
  
  if(mkfifo(req_pipe_path, 0777) < 0)  return 1;
  if(mkfifo(resp_pipe_path, 0777) < 0)  return 1;
  if(mkfifo(notif_pipe_path, 0777) < 0)  return 1;

  if ((freg = open (reg_pipe_path,O_WRONLY))<0) exit(1);

  pad_pipe_path(padded_req,req_pipe_path);
  pad_pipe_path(padded_resp,resp_pipe_path);
  pad_pipe_path(padded_notif,notif_pipe_path);

  
  sprintf(connect_message,"1%s%s%s",padded_req,padded_resp,padded_notif);
  write(freg,connect_message,MAX_CONNECT_MESSAGE_SIZE);

  // TODO: connect
  return 0;
}
 
int kvs_disconnect(void) {

  
  return 0;
}

int kvs_subscribe(const char* key) {
  // send subscribe message to request pipe and wait for response in response pipe
  return 0;
}

int kvs_unsubscribe(const char* key) {
    // send unsubscribe message to request pipe and wait for response in response pipe
  return 0;
}


