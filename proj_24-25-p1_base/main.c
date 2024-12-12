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

int pid_counts = 0; //Counter for the number of child processes created
int thread_count = 0; //Counter for the number of threads created

//Struct that caries the arguments for the function job_thread_handler
typedef struct {
  int fd;
  int fd2;
  char file_name [MAX_JOB_FILE_NAME_SIZE];
  int MAX_BACKUPS;
} ThreadArgs;

//Function that processes a .job file (fd) and writes its output in a corresponding .out file (fd2)
void job_handler(int fd, int fd2, const char* file_name, int MAX_BACKUPS) {
    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    size_t num_pairs;
    int num_backups = 0;
    unsigned int delay;
    char bck_number[4] = "";

    for (;;) {
        switch (get_next(fd)) {
            case CMD_WRITE:
                num_pairs = parse_write(fd, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);
                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command while writing. See HELP for usage\n");
                    continue;
                }

                if (kvs_write(num_pairs, keys, values)) {
                    fprintf(stderr, "Failed to write pair\n");
                }
                break;

            case CMD_READ:
                num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);
                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command while reading. See HELP for usage\n");
                    continue;
                }

                if (kvs_read(fd2, num_pairs, keys)) {
                    fprintf(stderr, "Failed to read pair\n");
                }
                break;

            case CMD_DELETE:
                num_pairs = parse_read_delete(fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

                if (num_pairs == 0) {
                    fprintf(stderr, "Invalid command while deleting. See HELP for usage\n");
                    continue;
                }

                if (kvs_delete(fd2, num_pairs, keys)) {
                    fprintf(stderr, "Failed to delete pair\n");
                }
                break;

            case CMD_SHOW:
                kvs_show(fd2);
                break;

            case CMD_WAIT:
                if (parse_wait(fd, &delay, NULL) == -1) {
                    fprintf(stderr, "Invalid command while waiting. See HELP for usage\n");
                    continue;
                }

                if (delay > 0) {
                    printf("Waiting...\n");
                    kvs_wait(delay);
                }
                break;

            case CMD_BACKUP:
                {
                    char file_bck[MAX_JOB_FILE_NAME_SIZE] = "";
                    pid_counts++;  
                    num_backups++; 
                    strncpy(file_bck, file_name, strlen(file_name) - 4);
                    strcat(file_bck, "-");
                    sprintf(bck_number, "%d", num_backups);
                    strcat(file_bck, bck_number);
                    strcat(file_bck, ".bck");

                    int fd3 = open(file_bck, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
                    if (fd3 < 0) {
                        perror("Opening error in .out file.");
                        return;
                    }

                    if (kvs_backup(fd3, pid_counts, MAX_BACKUPS)) {
                        fprintf(stderr, "Failed to perform backup.\n");
                    }

                    close(fd3);
                }
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
                    "  BACKUP\n"
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
    close(fd); 
    close(fd2);
}

//Function that unfolds the (void*)args passed to the thread into the arguments for the job_handler function
void* job_thread_handler(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    job_handler(args->fd, args->fd2, args->file_name, args->MAX_BACKUPS);
    return NULL;
}


int main(int argc, char* argv[]) {

  //Handling of inproper inputs by the user
  if(argc != 4){
    fprintf(stderr, "Propper usage is: ./kvs dirpath MAX_BACKUPS MAX_THREADS\n");
    return 1;
  }

  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }

  int MAX_BACKUPS,MAX_THREADS;
  DIR* dirp;
  struct dirent *dp;

  fflush(stdout);

  dirp = opendir(argv[1]);
  MAX_BACKUPS = atoi(argv[2]);
  MAX_THREADS = atoi(argv[3]);

  pthread_t threads[MAX_THREADS];
  ThreadArgs thread_args[MAX_THREADS];


  if (dirp == NULL){
    perror("Failure at opening directory"); 
    return EXIT_FAILURE;
  }
  

  for (;;){
    char file_name [MAX_JOB_FILE_NAME_SIZE] = "";
    char file_out [MAX_JOB_FILE_NAME_SIZE] = "";
    int fd,fd2;


    dp = readdir(dirp);

    if (dp == NULL){
      break;
    }

    //Skip . and .. files in the opened directory
    if (strcmp(dp->d_name,".") == 0 || strcmp(dp->d_name,"..") == 0)
      continue;

    strcat(file_name,argv[1]);
    strcat(file_name,"/");
    strcat(file_name,dp->d_name);
    strncpy(file_out,file_name,strlen(file_name)-3);
    strcat(file_out,"out");

    const char *dot = strrchr(file_name,'.');

    //Verification that the current file is a .job and if so, open in it read mode
    if (strcmp(dot+1,"job")==0){
      printf("JOB\n");
      fd = open(file_name, O_RDONLY); 
    }
    
    if (fd < 0) {
        perror("Opening error in .job files.");
        return EXIT_FAILURE;
    }

    //Open the .out file on which the output for the corresponding .job file will be written
    if (strcmp(dot+1,"job")==0){
        fd2 = open(file_out, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
    }
    
    if (fd2 < 0) {
        perror("Opening error in .out file.");
        return EXIT_FAILURE;
    }

    ThreadArgs* args = &thread_args[thread_count % MAX_THREADS]; 
    args->fd = fd;
    args->fd2 = fd2;
    strncpy(args->file_name, file_name, MAX_JOB_FILE_NAME_SIZE - 1);
    args->file_name[MAX_JOB_FILE_NAME_SIZE - 1] = '\0';
    args->MAX_BACKUPS = MAX_BACKUPS;

    //Creation of threads until the MAX_THREADS limit is reached
    if (thread_count < MAX_THREADS) {
        pthread_create(&threads[thread_count], NULL, job_thread_handler, (void*)args);
        thread_count++;
    } else {
        //Await for a thread to be finished before creating a new one, to ensure only MAX_THREADS run at the same time
        pthread_join(threads[thread_count % MAX_THREADS], NULL);
        pthread_create(&threads[thread_count % MAX_THREADS], NULL, job_thread_handler, (void*)args);
        thread_count++;
    }

  }  

  //Join all created threads
  for (int i = 0; i < (thread_count < MAX_THREADS ? thread_count : MAX_THREADS); i++) {
    pthread_join(threads[i], NULL);
  }
  
  kvs_terminate();
  closedir(dirp);
}