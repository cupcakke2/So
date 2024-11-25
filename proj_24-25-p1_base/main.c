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


int main() {

  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }

  while (1) {
    char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
    char dirpath [MAX_JOB_FILE_NAME_SIZE]; //Duvido
    char temp [MAX_JOB_FILE_NAME_SIZE];
    int MAX_BACKUPS;
    char input[300]; //Alterar depois
    unsigned int delay;
    size_t num_pairs;
    DIR* dirp;
    struct dirent *dp;

    printf("> ");
    fflush(stdout);

    fgets(input,300,stdin);
    sscanf(input, "%s%d",dirpath,&MAX_BACKUPS);
  
    dirp = opendir(dirpath);

    
    if (dirp == NULL){
      perror("Failure at opening directory"); //ALTERAR
      return EXIT_FAILURE;
    }
    
   
    for (;;){
      errno = 0;

      dp = readdir(dirp);
      if (dp == NULL)
        break;

      if (strcmp(dp->d_name,".") == 0 || strcmp(dp->d_name,"..") == 0)
        continue;

      
      int fd = open("test.job", O_RDONLY);

      if (fd < 0) {
          perror("open error");
          return EXIT_FAILURE;
      }

      printf("yipee");

      int fd2 = open("jobs/test.out", O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
      if (fd2 < 0) {
          perror("open error");
          return EXIT_FAILURE;
      }


      printf("%s\n", dp->d_name);
      close(fd);
      close(fd2);
  
    }
  

    switch (get_next(STDIN_FILENO)) {
      case CMD_WRITE:
        num_pairs = parse_write(STDIN_FILENO, keys, values, MAX_WRITE_SIZE, MAX_STRING_SIZE);
        if (num_pairs == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_write(num_pairs, keys, values)) {
          fprintf(stderr, "Failed to write pair\n");
        }

        break;

      case CMD_READ:
        num_pairs = parse_read_delete(STDIN_FILENO, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

        if (num_pairs == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_read(num_pairs, keys)) {
          fprintf(stderr, "Failed to read pair\n");
        }
        break;

      case CMD_DELETE:
        num_pairs = parse_read_delete(STDIN_FILENO, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);

        if (num_pairs == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (kvs_delete(num_pairs, keys)) {
          fprintf(stderr, "Failed to delete pair\n");
        }
        break;

      case CMD_SHOW:

        kvs_show();
        break;

      case CMD_WAIT:
        if (parse_wait(STDIN_FILENO, &delay, NULL) == -1) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          printf("Waiting...\n");
          kvs_wait(delay);
        }
        break;

      case CMD_BACKUP:

        if (kvs_backup()) {
          fprintf(stderr, "Failed to perform backup.\n");
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
            "  BACKUP\n" // Not implemented
            "  HELP\n"
        );

        break;
        
      case CMD_EMPTY:
        break;

      case EOC:
        kvs_terminate();
        return 0;
    }
  }
}
