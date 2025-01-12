#include "api.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "../common/constants.h"
#include "../common/protocol.h"
#include "../common/io.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

char global_request_pipe[MAX_PIPE_PATH_LENGTH];
char global_response_pipe[MAX_PIPE_PATH_LENGTH];
char global_notif_pipe[MAX_PIPE_PATH_LENGTH];

int intr = 0;

void pad_pipe_path(char* dest, const char* src) {
    strncpy(dest, src,  MAX_PIPE_PATH_LENGTH - 1); 
    dest[MAX_PIPE_PATH_LENGTH - 1] = '\0';       
    for (size_t i = strlen(src); i < MAX_PIPE_PATH_LENGTH; i++) {
        dest[i] = '\0';              
    }
}

void pad_key(char* dest, const char* src) {
    strncpy(dest, src,  MAX_KEY_SIZE - 1); 
    dest[MAX_KEY_SIZE - 1] = '\0';       
    for (size_t i = strlen(src); i < MAX_KEY_SIZE; i++) {
        dest[i] = '\0';              
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

  if ((freg = open (reg_pipe_path,O_RDWR))<0) exit(1);

  pad_pipe_path(padded_req,req_pipe_path);
  pad_pipe_path(padded_resp,resp_pipe_path);
  pad_pipe_path(padded_notif,notif_pipe_path);

  strcpy(global_request_pipe,padded_req);
  strcpy(global_response_pipe,padded_resp);
  strcpy(global_notif_pipe,padded_notif);

  size_t offset = 0;
  memcpy(connect_message + offset, "1", 1);  
  offset += 1;

  memcpy(connect_message + offset, padded_req, sizeof(padded_req) ); 
  offset += sizeof(padded_req);

  memcpy(connect_message + offset, padded_resp, sizeof(padded_resp) );  
  offset += sizeof(padded_resp);

  memcpy(connect_message + offset, padded_notif, sizeof(padded_notif));  
  offset += sizeof(padded_notif) ;

  
  
  write_all(freg,connect_message,MAX_CONNECT_MESSAGE_SIZE);

  return 0;
}
 
int kvs_disconnect(void) {

  int freq,fresp;
  char disconnect_message[MAX_DISCONECT_MESSAGE_SIZE];
  char disconnect_response[MAX_DISCONECT_RESPONSE_SIZE];

  disconnect_message[0]='2';
  disconnect_message[1]='\0';

  if ((freq = open (global_request_pipe,O_WRONLY))<0) exit(1);
  write_all(freq,disconnect_message,MAX_DISCONECT_MESSAGE_SIZE);

  if ((fresp = open (global_response_pipe,O_RDONLY))<0) exit(1);
  read_all(fresp,disconnect_response,MAX_DISCONECT_RESPONSE_SIZE,&intr);

  printf("Server returned %c for operation: disconnect\n",disconnect_response[1]);

  close(freq);
  close(fresp);
 

  if(disconnect_response[1]=='0'){
    return 0;
  }else{
    return 1;
  }
}

int kvs_subscribe(const char* key) {
  // send subscribe message to request pipe and wait for response in response pipe
  int freq,fresp;
  char subscribe_message[MAX_SUBSCRIBE_MESSAGE_SIZE];
  char subscribe_response[MAX_SUBSCRIBE_RESPONSE_SIZE];
  char padded_key[MAX_KEY_SIZE];
  

  if ((freq = open (global_request_pipe,O_WRONLY))<0) exit(1);
  pad_key(padded_key,key);
  sprintf(subscribe_message,"3%s",padded_key);
  write_all(freq,subscribe_message,MAX_SUBSCRIBE_MESSAGE_SIZE);

  if ((fresp = open (global_response_pipe,O_RDONLY))<0) exit(1);
  read_all(fresp,subscribe_response,MAX_SUBSCRIBE_RESPONSE_SIZE,&intr);

  printf("Server returned %c for operation: subscribe\n",subscribe_response[1]);

  close(freq);
  close(fresp);
 
  
  if(subscribe_response[1]=='0'){
    return 0;
  }else{
    return 1;
  }
}

int kvs_unsubscribe(const char* key) {
    // send unsubscribe message to request pipe and wait for response in response pipe
    // send subscribe message to request pipe and wait for response in response pipe
  int freq,fresp;
  char unsubscribe_message[MAX_UNSUBSCRIBE_MESSAGE_SIZE];
  char unsubscribe_response[MAX_UNSUBSCRIBE_RESPONSE_SIZE];
  char padded_key[MAX_KEY_SIZE];
  

  if ((freq = open (global_request_pipe,O_WRONLY))<0) exit(1);
  pad_key(padded_key,key);
  sprintf(unsubscribe_message,"4%s",padded_key);
  write_all(freq,unsubscribe_message,MAX_UNSUBSCRIBE_MESSAGE_SIZE);

  if ((fresp = open (global_response_pipe,O_RDONLY))<0) exit(1);
  read_all(fresp,unsubscribe_response,MAX_UNSUBSCRIBE_RESPONSE_SIZE,&intr);

  printf("Server returned %c for operation: unsubscribe\n",unsubscribe_response[1]);

  close(freq);
  close(fresp);
 
  if(unsubscribe_response[1]=='0'){
    return 0;
  }else{
    return 1;
  }

  return 0;
}


