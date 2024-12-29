#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "parser.h"
#include "api.h"
#include "../common/constants.h"
#include "../common/io.h"
#include "../common/io.c"


int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <client_unique_id> <register_pipe_path>\n", argv[0]);
    return 1;
  }

  


  char reg_pipe_path[MAX_PIPE_PATH_LENGTH] = "/tmp/";
  char req_pipe_path[MAX_PIPE_PATH_LENGTH] = "/tmp/req";
  char resp_pipe_path[MAX_PIPE_PATH_LENGTH] = "/tmp/resp";
  char notif_pipe_path[MAX_PIPE_PATH_LENGTH] = "/tmp/notif";
  char keys[MAX_NUMBER_SUB][MAX_STRING_SIZE] = {0};
  char connect_message[MAX_CONNECT_MESSAGE_SIZE] = "";
  unsigned int delay_ms;
  size_t num;
  int freg,freq,fresp,fnotif;

  strncat(req_pipe_path, argv[1], strlen(argv[1]) * sizeof(char));
  strncat(resp_pipe_path, argv[1], strlen(argv[1]) * sizeof(char));
  strncat(notif_pipe_path, argv[1], strlen(argv[1]) * sizeof(char));
  strncat(reg_pipe_path, argv[2], strlen(argv[2]) * sizeof(char));
  
  
 
  kvs_connect(req_pipe_path,resp_pipe_path,reg_pipe_path,notif_pipe_path);


  // TODO open pipes
  if ((freq = open (reg_pipe_path,O_RDWR))<0) exit(1);
  if ((fresp = open (reg_pipe_path,O_RDWR))<0) exit(1);
  if ((fnotif = open (reg_pipe_path,O_RDWR))<0) exit(1);
  if ((freg = open (reg_pipe_path,O_WRONLY))<0) exit(1);

  while(strlen(req_pipe_path)!=MAX_PIPE_PATH_LENGTH){
    strcat(req_pipe_path,"0");
  }
   while(strlen(resp_pipe_path)!=MAX_PIPE_PATH_LENGTH){
    strcat(resp_pipe_path,"0");
  }
   while(strlen(notif_pipe_path)!=MAX_PIPE_PATH_LENGTH){
    strcat(notif_pipe_path,"0");
  }
  
  sprintf(connect_message,"1|%s|%s|%s",req_pipe_path,resp_pipe_path,notif_pipe_path);
  printf("%s\n",connect_message);
  write(freg,connect_message,MAX_CONNECT_MESSAGE_SIZE);

  
  while (1) {
    switch (get_next(STDIN_FILENO)) {
      case CMD_DISCONNECT:
        if (kvs_disconnect() != 0) {
          fprintf(stderr, "Failed to disconnect to the server\n");
          return 1;
        }
        // TODO: end notifications thread
        printf("Disconnected from server\n");
        return 0;

      case CMD_SUBSCRIBE:
        num = parse_list(STDIN_FILENO, keys, 1, MAX_STRING_SIZE);
        if (num == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }
         
        if (kvs_subscribe(keys[0])) {
            fprintf(stderr, "Command subscribe failed\n");
        }

        break;

      case CMD_UNSUBSCRIBE:
        num = parse_list(STDIN_FILENO, keys, 1, MAX_STRING_SIZE);
        if (num == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }
         
        if (kvs_unsubscribe(keys[0])) {
            fprintf(stderr, "Command subscribe failed\n");
        }

        break;

      case CMD_DELAY:
        if (parse_delay(STDIN_FILENO, &delay_ms) == -1) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay_ms > 0) {
            printf("Waiting...\n");
            delay(delay_ms);
        }
        break;

      case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_EMPTY:
        break;

      case EOC:
        // input should end in a disconnect, or it will loop here forever
        break;
    }
  }
  close(freg);

}