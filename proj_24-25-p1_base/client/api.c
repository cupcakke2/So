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



int kvs_connect(char const* req_pipe_path, char const* resp_pipe_path, char const* reg_pipe_path,
                char const* notif_pipe_path) {

  int freg;              
  char connect_message[MAX_CONNECT_MESSAGE_SIZE] = "";
  unlink(req_pipe_path);
  unlink(resp_pipe_path);
  unlink(notif_pipe_path);
  
  if(mkfifo(req_pipe_path, 0777) < 0)  return 1;
  if(mkfifo(resp_pipe_path, 0777) < 0)  return 1;
  if(mkfifo(notif_pipe_path, 0777) < 0)  return 1;

  if ((freg = open (reg_pipe_path,O_WRONLY))<0) exit(1);

  sprintf(connect_message,"1%s%s%s",req_pipe_path,resp_pipe_path,notif_pipe_path);
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


