#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include "constants.h"
#include "../common/constants.h"
#include "io.h"
#include "../common/io.c"
#include "operations.h"
#include "parser.h"
#include "pthread.h"
#include <errno.h>
#include <time.h>
#include <signal.h>

struct SharedData {
  DIR *dir;
  char *dir_name;
  pthread_mutex_t directory_mutex;
};

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t n_current_backups_lock = PTHREAD_MUTEX_INITIALIZER;

size_t active_backups = 0; // Number of active backups
size_t max_backups;        // Maximum allowed simultaneous backups
size_t max_threads;        // Maximum allowed simultaneous threads
char *jobs_directory = NULL;
char global_keys[MAX_KEY_SIZE][MAX_NUMBER_SUB]; //In our one client solution, subscriptions of said client are kept in this array
char reg_pipe_path[MAX_PIPE_PATH_LENGTH]="/tmp/";
char req_pipe_path[MAX_PIPE_PATH_LENGTH];
char resp_pipe_path[MAX_PIPE_PATH_LENGTH];
char notif_pipe_path[MAX_PIPE_PATH_LENGTH];
int fresp; 
int intr = 0; //Variable set to one if read_all was interrupted
int counter_keys = 0;

//When the process detetects the custom SIGUSR1, it will erase the subscriptions of the client and unlink its notif and resp pipes.
void handle_sigusr1(int sig) {

  sig++;//Strictly here to avoid unused parameter warning during compilation

  for(int i = 0; i<MAX_NUMBER_SUB; i++){
    memset(global_keys[i], 0, MAX_STRING_SIZE); //memset is async-signal safe!
  }

  unlink(notif_pipe_path);
  unlink(resp_pipe_path);
}

//When the use presses ctr+C in the terminal we still want to close all pipes before exiting the terminal as usual
void handle_sigint(int sig) {

  sig++; //Strictly here to avoid unused parameter warning during compilation

  unlink(req_pipe_path);
  unlink(reg_pipe_path);
  unlink(resp_pipe_path);
  unlink(notif_pipe_path);

  signal(SIGINT, SIG_DFL);  // Set the handler to default 
  raise(SIGINT);
}



int filter_job_files(const struct dirent *entry) {
  const char *dot = strrchr(entry->d_name, '.');
  if (dot != NULL && strcmp(dot, ".job") == 0) {
    return 1; // Keep this file (it has the .job extension)
  }
  return 0;
}

static int entry_files(const char *dir, struct dirent *entry, char *in_path,
                       char *out_path) {
  const char *dot = strrchr(entry->d_name, '.');
  if (dot == NULL || dot == entry->d_name || strlen(dot) != 4 ||
      strcmp(dot, ".job")) {
    return 1;
  }

  if (strlen(entry->d_name) + strlen(dir) + 2 > MAX_JOB_FILE_NAME_SIZE) {
    fprintf(stderr, "%s/%s\n", dir, entry->d_name);
    return 1;
  }

  strcpy(in_path, dir);
  strcat(in_path, "/");
  strcat(in_path, entry->d_name);

  strcpy(out_path, in_path);
  strcpy(strrchr(out_path, '.'), ".out");

  return 0;
}

static int run_job(int in_fd, int out_fd, char *filename) {
  size_t file_backups = 0;
  while (1) {
    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    unsigned int delay;
    size_t num_pairs;

    switch (get_next(in_fd)) {
    case CMD_WRITE:
      num_pairs =
          parse_write(in_fd, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);
      if (num_pairs == 0) {
        write_str(STDERR_FILENO, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (kvs_write(num_pairs, keys, values)) {
        write_str(STDERR_FILENO, "Failed to write pair\n");
      }
      break;

    case CMD_READ:
      num_pairs =
          parse_read_delete(in_fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

      if (num_pairs == 0) {
        write_str(STDERR_FILENO, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (kvs_read(num_pairs, keys, out_fd)) {
        write_str(STDERR_FILENO, "Failed to read pair\n");
      }
      break;

    case CMD_DELETE:
      num_pairs =
          parse_read_delete(in_fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

      if (num_pairs == 0) {
        write_str(STDERR_FILENO, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (kvs_delete(num_pairs, keys, out_fd)) {
        write_str(STDERR_FILENO, "Failed to delete pair\n");
      }
      break;

    case CMD_SHOW:
      kvs_show(out_fd);
      break;

    case CMD_WAIT:
      if (parse_wait(in_fd, &delay, NULL) == -1) {
        write_str(STDERR_FILENO, "Invalid command. See HELP for usage\n");
        continue;
      }

      if (delay > 0) {
        printf("Waiting %d seconds\n", delay / 1000);
        kvs_wait(delay);
      }
      break;

    case CMD_BACKUP:
      pthread_mutex_lock(&n_current_backups_lock);
      if (active_backups >= max_backups) {
        wait(NULL);
      } else {
        active_backups++;
      }
      pthread_mutex_unlock(&n_current_backups_lock);
      int aux = kvs_backup(++file_backups, filename, jobs_directory);

      if (aux < 0) {
        write_str(STDERR_FILENO, "Failed to do backup\n");
      } else if (aux == 1) {
        return 1;
      }
      break;

    case CMD_INVALID:
      write_str(STDERR_FILENO, "Invalid command. See HELP for usage\n");
      break;

    case CMD_HELP:
      write_str(STDOUT_FILENO,
                "Available commands:\n"
                "  WRITE [(key,value)(key2,value2),...]\n"
                "  READ [key,key2,...]\n"
                "  DELETE [key,key2,...]\n"
                "  SHOW\n"
                "  WAIT <delay_ms>\n"
                "  BACKUP\n"
                "  HELP\n");

      break;

    case CMD_EMPTY:
      break;

    case EOC:
      printf("EOF\n"); //Inform that the .jobs was processed until the end
      return 0;
    }
  }
}

// frees arguments
static void *get_file(void *arguments) {
  struct SharedData *thread_data = (struct SharedData *)arguments;
  DIR *dir = thread_data->dir;
  char *dir_name = thread_data->dir_name;

  delay(100);

  if (pthread_mutex_lock(&thread_data->directory_mutex) != 0) {
    fprintf(stderr, "Thread failed to lock directory_mutex\n");
    return NULL;
  }

  struct dirent *entry;
  char in_path[MAX_JOB_FILE_NAME_SIZE], out_path[MAX_JOB_FILE_NAME_SIZE];
  while ((entry = readdir(dir)) != NULL) {
    if (entry_files(dir_name, entry, in_path, out_path)) {
      continue;
    }

    if (pthread_mutex_unlock(&thread_data->directory_mutex) != 0) {
      fprintf(stderr, "Thread failed to unlock directory_mutex\n");
      return NULL;
    }

    int in_fd = open(in_path, O_RDONLY);
    if (in_fd == -1) {
      write_str(STDERR_FILENO, "Failed to open input file: ");
      write_str(STDERR_FILENO, in_path);
      write_str(STDERR_FILENO, "\n");
      pthread_exit(NULL);
    }

    int out_fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd == -1) {
      write_str(STDERR_FILENO, "Failed to open output file: ");
      write_str(STDERR_FILENO, out_path);
      write_str(STDERR_FILENO, "\n");
      pthread_exit(NULL);
    }

    int out = run_job(in_fd, out_fd, entry->d_name);

    close(in_fd);
    close(out_fd);

    if (out) {
      if (closedir(dir) == -1) {
        fprintf(stderr, "Failed to close directory\n");
        return 0;
      }

      exit(0);
    }

    if (pthread_mutex_lock(&thread_data->directory_mutex) != 0) {
      fprintf(stderr, "Thread failed to lock directory_mutex\n");
      return NULL;
    }
  }

  if (pthread_mutex_unlock(&thread_data->directory_mutex) != 0) {
    fprintf(stderr, "Thread failed to unlock directory_mutex\n");
    return NULL;
  }

  pthread_exit(NULL);
}

static void dispatch_threads(DIR *dir) {

  char connect_message[MAX_CONNECT_MESSAGE_SIZE]; 
  char connect_response[MAX_CONNECT_RESPONSE_SIZE];
  char connect_opcode;
  int freg;
  
  pthread_t *threads = malloc(max_threads * sizeof(pthread_t));

  if (threads == NULL) {
    fprintf(stderr, "Failed to allocate memory for threads\n");
    return;
  }

  struct SharedData thread_data = {dir, jobs_directory, PTHREAD_MUTEX_INITIALIZER};

  for (size_t i = 0; i < max_threads; i++) {
    if (pthread_create(&threads[i], NULL, get_file, (void *)&thread_data) !=
        0) {
      fprintf(stderr, "Failed to create thread %zu\n", i);
      pthread_mutex_destroy(&thread_data.directory_mutex);
      free(threads);
      return;
    }
  }

  //Opening the register pipe in RDWR mode so that it does not close when there are no clients writing in it
  if((freg = open(reg_pipe_path, O_RDWR)) < 0) exit(1);

  if(read_all(freg,connect_message,MAX_CONNECT_MESSAGE_SIZE,&intr) == -1){
    fprintf(stderr,"Failed to read from register pipe\n");
    return;
  }

  close(freg);

  for(size_t i = 0; i< sizeof(connect_message); i++){
        if (i == 0) {
            connect_opcode = connect_message[i];
        }
        else if (i>0 && i<=40){
            req_pipe_path[i-1] = connect_message[i];
        }
        else if (i>=41 && i<=80){
            resp_pipe_path[i-41] = connect_message[i];
        }
        else if (i>=81 && i<=120){
            notif_pipe_path[i-81] = connect_message[i];
        }
    }
   
  connect_response[0]=connect_opcode;

  if(sizeof(connect_message)!=121){
    connect_response[1]='1';
  }else{
    connect_response[1]='0';
  }

  connect_response[2]='\0';


  if ((fresp = open (resp_pipe_path,O_WRONLY))<0) exit(1);

  write_all(fresp,connect_response,MAX_CONNECT_RESPONSE_SIZE);
  close(fresp);

  for (unsigned int i = 0; i < max_threads; i++) {
    if (pthread_join(threads[i], NULL) != 0) {
      fprintf(stderr, "Failed to join thread %u\n", i);
      pthread_mutex_destroy(&thread_data.directory_mutex);
      free(threads);
      return;
    }
  }

  if (pthread_mutex_destroy(&thread_data.directory_mutex) != 0) {
    fprintf(stderr, "Failed to destroy directory_mutex\n");
  }

  free(threads);
  
}

int main(int argc, char **argv) {
  if (argc != 5) {
    write_str(STDERR_FILENO, "Usage: ");
    write_str(STDERR_FILENO, argv[0]);
    write_str(STDERR_FILENO, " <jobs_dir>");
    write_str(STDERR_FILENO, " <max_threads>");
    write_str(STDERR_FILENO, " <max_backups> ");
    write_str(STDERR_FILENO, " <register_pipe_path>\n");
    return 1;
  }

  /*
  
  Notification pipe test:
  In the example below we "pre-subscribe" (a,prev) and then when the .job file overwrites and then deletes the "a"
  key we receive the following message from the notification pipe:


  //Pre-subscription:
  strncpy(global_keys[0], "a", MAX_NUMBER_SUB);
  strncpy(global_keys[1], "prev", MAX_NUMBER_SUB);

  //Notif-pipe result:
  (a,anna)
  (a,DELETED)
  
  */
 

  jobs_directory = argv[1];

  char *endptr;
  
  max_backups = strtoul(argv[3], &endptr, 10);
  

  if (*endptr != '\0') {
    fprintf(stderr, "Invalid max_proc value\n");
    return 1;
  }

  max_threads = strtoul(argv[2], &endptr, 10);

  if (*endptr != '\0') {
    fprintf(stderr, "Invalid max_threads value\n");
    return 1;
  }

  if (max_backups <= 0) {
    write_str(STDERR_FILENO, "Invalid number of backups\n");
    return 0;
  }

  if (max_threads <= 0) {
    write_str(STDERR_FILENO, "Invalid number of threads\n");
    return 0;
  }

  if (sizeof(argv[4]) > MAX_PIPE_PATH_LENGTH) {
    write_str(STDERR_FILENO, "Name of the register pipe is too long\n");
    return 0;
  }

  
  strncat(reg_pipe_path, argv[4], strlen(argv[4]) * sizeof(char));

  unlink(reg_pipe_path);

  if(mkfifo(reg_pipe_path, 0777) < 0) exit (1);


  if (kvs_init()) {
    write_str(STDERR_FILENO, "Failed to initialize KVS\n");
    return 1;
  }

  DIR *dir = opendir(argv[1]);
  if (dir == NULL) {
    fprintf(stderr, "Failed to open directory: %s\n", argv[1]);
    return 0;
  }


  dispatch_threads(dir);

  if (closedir(dir) == -1) {
    fprintf(stderr, "Failed to close directory\n");
    return 0;
  }

  //Handling SIGUSR1
  if (signal(SIGUSR1, handle_sigusr1) == SIG_ERR) {
    perror("Unable to catch SIGUSR1");
    exit(1);
  }

  //Handling SIGPIPES that happen when we detect SIGUSR1 and we close the notif and resp pipes from the client
  //we want to ignore this SIGPIPE so that the server doesn't terminate and awaits the connection of another client
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    perror("Unable to ignore SIG_PIPE");
    exit(1);
  }

  //Handling SIGINT
  if (signal(SIGINT,handle_sigint) == SIG_ERR) {
    perror("Unable to ignore SIG_PIPE");
    exit(1);
  }

  next_client:
  int freq;
  char request[MAX_REQUEST_SIZE];
  char subscribe_key[MAX_KEY_SIZE];
  char unsubscribe_key[MAX_KEY_SIZE];
  char subscribe_response[MAX_SUBSCRIBE_RESPONSE_SIZE];
  char unsubscribe_response[MAX_UNSUBSCRIBE_RESPONSE_SIZE];
  char opcode;
  
  if ((freq = open (req_pipe_path,O_RDONLY))<0) exit(1);

  while(1){

    //We use read instead of read_all because it would block for disconnect request since they have only 2 + '\0'
    ssize_t bytes_read = read(freq,request,MAX_REQUEST_SIZE); 

    // Client closed the connection, so we await another client
    if (bytes_read == 0) {
      
      int freg;
      char connect_message[MAX_CONNECT_MESSAGE_SIZE]; 
      char connect_response[MAX_CONNECT_RESPONSE_SIZE];
      char connect_opcode;
      printf("Client disconnected, waiting for new client...\n");

      //Opening the register pipe in RDWR mode so that it does not close when there are no clients writing in it
      if((freg = open(reg_pipe_path, O_RDWR)) < 0) exit(1);

      if(read_all(freg,connect_message,MAX_CONNECT_MESSAGE_SIZE,&intr) == -1){
        fprintf(stderr,"Failed to read from register pipe\n");
        return 0;
      }

      close(freg);

      for(size_t i = 0; i< sizeof(connect_message); i++){
            if (i == 0) {
                connect_opcode = connect_message[i];
            }
            else if (i>0 && i<=40){
                req_pipe_path[i-1] = connect_message[i];
            }
            else if (i>=41 && i<=80){
                resp_pipe_path[i-41] = connect_message[i];
            }
            else if (i>=81 && i<=120){
                notif_pipe_path[i-81] = connect_message[i];
            }
        }
   
      connect_response[0]=connect_opcode;

      if(sizeof(connect_message)!=121){
        connect_response[1]='1';
      }else{
        connect_response[1]='0';
      }

       connect_response[2]='\0';


      if ((fresp = open (resp_pipe_path,O_WRONLY))<0) exit(1);

      write_all(fresp,connect_response,MAX_CONNECT_RESPONSE_SIZE);
      close(fresp);

      goto next_client; // Goes to the loop for processing requests for this new client
    }

    // Error during read operation
    if (bytes_read < 0) {
      perror("Error reading from pipe");
      break; 
    }

    //Disconnect case
    if(request[0] == '2'){

      char disconnect_response[MAX_DISCONNECT_RESPONSE_SIZE];

      unlink(req_pipe_path);
      unlink(notif_pipe_path);

      for(int i = 0; i<MAX_NUMBER_SUB; i++){
        strcpy(global_keys[i],"");
      }

      sprintf(disconnect_response,"%d%d",2,0);
      if ((fresp = open (resp_pipe_path,O_WRONLY))<0) exit(1);
      write_all(fresp,disconnect_response,MAX_DISCONNECT_RESPONSE_SIZE);
      close(fresp);
      unlink(resp_pipe_path);

    }

    //Subscribe case
    if(request[0] == '3'){

      int already_subscribed = 0;

      for(size_t i = 0; i< sizeof(request); i++){
        if (i == 0) {
            opcode = request[i];
        }
        else if (i>0 && i<=41){
            subscribe_key[i-1] = request[i];
        }
      }

      //Even if the key exists, we must fail the subscription to avoid duplicates
      if(exists_key(subscribe_key)){
        for (int i =0; i <MAX_NUMBER_SUB; i++){
          if(strcmp(subscribe_key,global_keys[i])==0){
            already_subscribed = 1;
          }
        }
        if(already_subscribed == 0){
          strcpy(global_keys[counter_keys],subscribe_key);
          counter_keys ++;
    
        }  
      }
      if(already_subscribed == 1){
        sprintf(subscribe_response,"%c%d",opcode,0);
        if ((fresp = open (resp_pipe_path,O_WRONLY))<0) exit(1);
        write_all(fresp,subscribe_response,MAX_SUBSCRIBE_RESPONSE_SIZE);
        close(fresp);
      }else{
        sprintf(subscribe_response,"%c%d",opcode,exists_key(subscribe_key));
        if ((fresp = open (resp_pipe_path,O_WRONLY))<0) exit(1);
        write_all(fresp,subscribe_response,MAX_SUBSCRIBE_RESPONSE_SIZE);
        close(fresp);
      }
      
    }

    //Unsubcribe case
    if(request[0] == '4'){
      int exists = 0;
      
      for(size_t i = 0; i< sizeof(request); i++){
        if (i == 0) {
          opcode = request[i];
        }
        else if (i>0 && i<=41){
          unsubscribe_key[i-1] = request[i];
        }
      }

      for(int i=0; i<MAX_NUMBER_SUB; i++){
        if (strcmp(global_keys[i],unsubscribe_key) == 0){
          exists = 1;
          strcpy(global_keys[i],"");
          sprintf(unsubscribe_response,"%c%d",opcode,0);
          if ((fresp = open (resp_pipe_path,O_WRONLY))<0) exit(1);
          write_all(fresp,unsubscribe_response,MAX_UNSUBSCRIBE_RESPONSE_SIZE);
          close(fresp);
        }
      }

      if(exists==0){
        sprintf(unsubscribe_response,"%c%d",opcode,1);
        if ((fresp = open (resp_pipe_path,O_WRONLY))<0) exit(1);
        write_all(fresp,unsubscribe_response,MAX_UNSUBSCRIBE_RESPONSE_SIZE);
        close(fresp);
      }
    }
  }


  while (active_backups > 0) {
    wait(NULL);
    active_backups--;
  }

  kvs_terminate();

  return 0;
}
