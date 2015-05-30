
#include <cstdio>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

void *connection_handler(void *socket) {
  int clientfd = *(int*)socket;
  int result;

  size_t bufferLen = 2000;
  char buffer[bufferLen];

  ssize_t len;
  while((len = recv(clientfd, buffer, bufferLen-1, 0)) > 0) {
    buffer[len] = '\0';
    printf("Recv: %s\n", &buffer[0]); 

    result = send(clientfd, buffer, (int) len, 0);
    if (result == -1) {
      printf("Send error: %s\n", strerror(errno));
      pthread_exit((void*)1);;
    }

    if (len == -1) {
      printf("Recv error: %s \n", strerror(errno));
      return (void*)1;
    }
  }

  printf("Client diconnection\n");

  return 0;
};
