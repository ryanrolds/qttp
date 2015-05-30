
#include <condition_variable>
#include <cstdio>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

void *accept_handler(void * arg);
void *connection_handler(void *);

int qttp() {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  const char* port = "8080";
  struct addrinfo *address;
  int result = getaddrinfo(NULL, port, &hints, &address);
  if (result != 0) {
    printf("getaddrinfo: %s \n", strerror(errno));
    return 1;
  }

  int socketfd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
  printf("Socket fd: %u\n", socketfd);

  result = bind(socketfd, address->ai_addr, address->ai_addrlen); 
  printf("Bind result: %i\n", result);
  if (result != 0) {
    printf("Bind error: %s \n", strerror(errno));
    return 1;
  }

  listen(socketfd, 3);

  struct sockaddr client = {0};
  socklen_t clientLen = sizeof(struct sockaddr);;
  int clientfd; 

  while((clientfd = accept(socketfd, &client, &clientLen)) != -1) {
    printf("Client connection\n");
    
    pthread_t threadId;
    if (pthread_create(&threadId, NULL, connection_handler, &clientfd) > 0) {
      printf("Thread creation failed");
      return 1;
    }

    printf("Passed to thread");
  }
  
  if (clientfd == -1) {
    printf("Accept error: %s \n", strerror(errno));
    return 1;
  }

  //free(clientfd);
  // shutdown(clientfd);
  //close(socketfd);

  //freeaddrinfo(address);   

  return 0;
}
