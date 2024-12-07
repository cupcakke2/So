#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include "constants.h"
#include "parser.h"
#include "operations.h"
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include "structs.h"




void *kvs_write_wrapper(void *args) {
    write_args_t *write_args = (write_args_t *)args;
    int result = kvs_write(write_args->num_pairs, write_args->keys, write_args->values, write_args->file);
    return (void *)(intptr_t)result; // Cast result to void * for return
}


int main(int argc, char* argv[]) {

  if(argc != 4){
    fprintf(stderr, "Propper usage is: ./kvs dirpath MAX_BACKUPS MAX_THREADS\n");
    return 1;
  }

  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }

  char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
  char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
  int MAX_BACKUPS;
  int MAX_THREADS;
  unsigned int delay;
  int pid_counts = 0;
  int thread_counts = 0;
  size_t num_pairs;
  DIR* dirp;
  struct dirent *dp;

  fflush(stdout);

  dirp = opendir(argv[1]);
  MAX_BACKUPS = atoi(argv[2]);
  MAX_THREADS = atoi(argv[3]);

  pthread_t tid[MAX_THREADS];
  


  
  if (dirp == NULL){
    perror("Failure at opening directory"); 
    return EXIT_FAILURE;
  }
  

  for (;;){
    char file_name [MAX_JOB_FILE_NAME_SIZE] = "";
    char file_out [MAX_JOB_FILE_NAME_SIZE] = "";
    int num_backups = 0;
    char bck_number [4] = "";
    errno = 0;


    dp = readdir(dirp);

    if (dp == NULL){
      kvs_terminate();
      break;
    }

    if (strcmp(dp->d_name,".") == 0 || strcmp(dp->d_name,"..") == 0)
      continue;

    strcat(file_name,argv[1]);
    strcat(file_name,"/");
    strcat(file_name,dp->d_name);
    strncpy(file_out,file_name,strlen(file_name)-3);
    strcat(file_out,"out");

    const char *dot = strrchr(file_name,'.');

    fd_with_mutex file;

    if (strcmp(dot+1,"job")==0){
      file.fd = open(file_name, O_RDONLY); 
    }

    
    if (file.fd < 0) {
        perror("Opening error in .job files.");
        return EXIT_FAILURE;
    }

    int fd2 = open(file_out, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);

    
    if (fd2 < 0) {
        perror("Opening error in .out file.");
        return EXIT_FAILURE;
    }

    for(;;){
      pthread_rwlock_init(&file.rwlock, NULL);
      switch (get_next(file.fd)) {
        case CMD_WRITE:
      
          num_pairs = parse_write(file.fd, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);
          if (num_pairs == 0) {
            fprintf(stderr, "Invalid command while writing. See HELP for usage\n");
            continue;
          }

          
          if(thread_counts < MAX_THREADS){
            write_args_t write_args = {.num_pairs=num_pairs};
    

            memcpy(write_args.keys, keys, sizeof(keys));
            memcpy(write_args.values, values, sizeof(values));
            write_args.file = file;
            write_args.file.fd = file.fd;
            write_args.file.rwlock = file.rwlock;
            
            if (pthread_create(&tid[thread_counts],0,kvs_write_wrapper,(void *)&write_args) != 0) {
              fprintf(stderr, "failed to create thread: %s\n", strerror(errno));
              exit(EXIT_FAILURE);
            }
          } else {
            if(kvs_write(num_pairs, keys, values,file)!=0){
              fprintf(stderr, "Failed to write pair\n");
            }
          }

          break;

        case CMD_READ:
          num_pairs = parse_read_delete(file.fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);
          if (num_pairs == 0) {
            fprintf(stderr, "Invalid command while reading. See HELP for usage\n");
            continue;
          }

          if (kvs_read(fd2,num_pairs,keys)) {
            fprintf(stderr, "Failed to read pair\n");
          }
          break;

        case CMD_DELETE:
          num_pairs = parse_read_delete(file.fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

          if (num_pairs == 0) {
            fprintf(stderr, "Invalid command while deleting. See HELP for usage\n");
            continue;
          }

          if (kvs_delete(fd2,num_pairs, keys)) {
            fprintf(stderr, "Failed to delete pair\n");
          }
          break;

        case CMD_SHOW:

          kvs_show(fd2);
          break;

        case CMD_WAIT:
          if (parse_wait(file.fd, &delay, NULL) == -1) {
            fprintf(stderr, "Invalid command while waiting. See HELP for usage\n");
            continue;
          }

          if (delay > 0) {
            printf("Waiting...\n");
            kvs_wait(delay);
          }
          break;

        case CMD_BACKUP:

          char file_bck [MAX_JOB_FILE_NAME_SIZE] = "";
          pid_counts++;
          num_backups++;
          strncpy(file_bck,file_out,strlen(file_name)-4);
          strcat(file_bck,"-");
          sprintf(bck_number, "%d", num_backups); 
          strcat(file_bck,bck_number);
          strcat(file_bck, ".bck");


          int fd3 = open(file_bck, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);

          

          if (fd3 < 0) {
              perror("Opening error in .out file.");
              return EXIT_FAILURE;
          }


          if (kvs_backup(fd3,pid_counts,MAX_BACKUPS)) {
            fprintf(stderr, "Failed to perform backup.\n");
          }

          strcpy(file_bck,"");
          strcpy(bck_number,"");
          break;

        case CMD_INVALID:
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          break;

        case CMD_HELP:
          printf( 
              "Available commands:\n"
              "  WRITE [(key,value),(key2,value2),...]\n"
              "  READ [key,key2,...]\n"
              "  DELETE [key,key2,...]\n"
              "  SHOW\n"
              "  WAIT <delay_ms>\n"
              "  BACKUP\n" // Not implemented
              "  HELP\n"
          );

          break;
          
        case CMD_EMPTY:
          break;

        case EOC:
          goto next_file;
          break;
      }
    }
  next_file:
  close(file.fd);
  pthread_rwlock_destroy(&file.rwlock);
  close(fd2);  
  num_backups=0;
  thread_counts ++;
  }  
  for (int i = 0; i < thread_counts; ++i) {
    pthread_join(tid[i], NULL);
  }
  closedir(dirp);
}