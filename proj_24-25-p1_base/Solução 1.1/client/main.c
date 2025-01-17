#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "parser.h"
#include "api.h"
#include "../common/constants.h"
#include "../common/io.h"
#include "../common/io.c"

int intr_m = 0; //Variable set to 1 if read_all is interrupted
char req_pipe_path[MAX_PIPE_PATH_LENGTH] = "/tmp/req";
char resp_pipe_path[MAX_PIPE_PATH_LENGTH] = "/tmp/resp";
char notif_pipe_path[MAX_PIPE_PATH_LENGTH] = "/tmp/notif";
pthread_t notif_thread; //main thread to handle the notification pipe

//Handles a crt+C in the client terminal by closing req, resp and notif path before proceding with the usual SIGINT, we also end the notif thread
void handle_sigint(int sig) {

  sig++; //Strictly here to avoid unused parameter warning during compilation

  unlink(req_pipe_path);
  unlink(resp_pipe_path);
  unlink(notif_pipe_path);

  //Ending notification thread
  if (pthread_cancel(notif_thread)) {
    return ;
  }

  signal(SIGINT, SIG_DFL);  // Set the handler to default 
  raise(SIGINT);
}

//Similar to the handling of sigint. When a SIGURS1 is received the resp and notif pipes are destroyed leading to a SIGPIPE
//We use this to terminate the client in that case.
//Another possible SIGPIPE is when the server destroys the reg pipe, hence the need to also unlink the req,resp and notif pipes.
//We also end the notif thread here
void handle_sigpipe(int sig) {

  sig++; //Strictly here to avoid unused parameter warning during compilation

  unlink(req_pipe_path);
  unlink(resp_pipe_path);
  unlink(notif_pipe_path);

  //Ending notification thread
  if (pthread_cancel(notif_thread)) {
    return ;
  }

  signal(SIGINT, SIG_DFL);  // Set the handler to default 
  raise(SIGINT);
}

//Function used by the second thread of the client to handle overwritten/deleted keys that are sent trough the notif pipe
void *notification_handler(void *arg) {
  
  char notification[MAX_NOTIFICATION_SIZE];

  char *notif_pipe = (char *)arg;
  int fnotif;

  if ((fnotif = open (notif_pipe,O_RDONLY))<0) exit(1);

  while (1) {
    ssize_t bytes_read = read_all(fnotif,notification,MAX_NOTIFICATION_SIZE,&intr_m);
    if (bytes_read > 0) {
      printf("%s\n", notification);
    } else if (bytes_read == 0) {
      continue;
    } else {
      perror("Error reading from notification pipe");
      break;
    }
  }
  close(fnotif);
  return NULL;
}

int main(int argc, char* argv[]) {

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <client_unique_id> <register_pipe_path>\n", argv[0]);
    return 1;
  }

  char reg_pipe_path[MAX_PIPE_PATH_LENGTH] = "/tmp/";
  char connect_response[MAX_CONNECT_RESPONSE_SIZE];
  char keys[MAX_NUMBER_SUB][MAX_STRING_SIZE] = {0};
  unsigned int delay_ms;
  size_t num;
  int freq,fresp,fnotif;

 

  fflush(stdout);
  strncat(req_pipe_path, argv[1], strlen(argv[1]) * sizeof(char));
  strncat(resp_pipe_path, argv[1], strlen(argv[1]) * sizeof(char));
  strncat(notif_pipe_path, argv[1], strlen(argv[1]) * sizeof(char));
  strncat(reg_pipe_path, argv[2], strlen(argv[2]) * sizeof(char));
  
  
 
  if(kvs_connect(req_pipe_path,resp_pipe_path,reg_pipe_path,notif_pipe_path)){
    printf("Server returned 1 for operation: connect");
  }

  
  //Opening pipes
  if ((freq = open (req_pipe_path,O_RDWR))<0) exit(1);
  if ((fresp = open (resp_pipe_path,O_RDONLY))<0) exit(1);
  if ((fnotif = open (notif_pipe_path,O_RDWR))<0) exit(1);

  read(fresp,connect_response,MAX_CONNECT_RESPONSE_SIZE);

  printf("Server returned %c for operation: connect\n",connect_response[1]);

  
  if (pthread_create(&notif_thread, NULL, notification_handler, (void *)notif_pipe_path)) {
    fprintf(stderr, "Failed to create notification handler thread\n");
    return 1;
  }

  //Handling SIGINT
  if (signal(SIGINT,handle_sigint) == SIG_ERR) {
    perror("Unable to handle SIGINT");
    exit(1);
  }
 
  //Handling SIGPIPE
  if (signal(SIGPIPE,handle_sigpipe) == SIG_ERR) {
    perror("Unable to handle_sigpipe");
    exit(1);
  }

  while (1) {
    switch (get_next(STDIN_FILENO)) {
      case CMD_DISCONNECT:
      
        if (kvs_disconnect()) {
          fprintf(stderr, "Failed to disconnect to the server\n");
          return 1;
        }
      
        printf("Disconnected from server\n");

        //Ending notification thread
        if (pthread_cancel(notif_thread)) {
            fprintf(stderr, "Error canceling thread\n");
            return 1;
        }
        return 0;

      case CMD_SUBSCRIBE:

        num = parse_list(STDIN_FILENO, keys, 1, MAX_STRING_SIZE);
        if (num == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

  
        if (!kvs_subscribe(keys[0])) {
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
            fprintf(stderr, "Command unsubscribe failed\n");
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
}
