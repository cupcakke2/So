#include "api.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "../common/constants.h"
#include "../common/protocol.h"

int kvs_connect(char const* req_pipe_path, char const* resp_pipe_path, char const* reg_pipe_path,
                char const* notif_pipe_path) {

   if(mkfifo(req_pipe_path, 0777) < 0)  exit (1);
   if(mkfifo(resp_pipe_path, 0777) < 0)  exit (1);
   if(mkfifo(notif_pipe_path, 0777) < 0)  exit (1);
  // TODO: connect
  return 0;
}
 
int kvs_disconnect(void) {
  // close pipes and unlink pipe files
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


