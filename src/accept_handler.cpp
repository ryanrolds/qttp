#include "connection_queue.h"
#include "accept_handler.h"

#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <thread>

void *accept_handler(accept_handler_args *args) {
  ConnectionQueue *queue = args->queue;

  int clientfd;
  struct sockaddr client = {0};
  socklen_t clientLen = sizeof(struct sockaddr);;

  fd_set read_fds;
  struct timeval time;
  int select_result;

  while(queue->running()) {
    FD_ZERO(&read_fds);
    FD_SET(*args->socketfd, &read_fds);
    
    time.tv_sec = 1;
    time.tv_usec = 0;

    select_result = select(*args->socketfd + 1, &read_fds, NULL, NULL, &time);
    if (select_result < 0) {
      std::cout << "Select error " << strerror(errno) << "\n";
      // Error
    }

    if (select_result == 0) {
      std::cout << "Select timout\n";
      // timeout
      continue;
    }

    if (FD_ISSET(*args->socketfd, &read_fds) == false) {
      continue;
    }

    std::cout << "Connection ready\n";

    clientfd = accept(*args->socketfd, &client, &clientLen);
    if (clientfd == -1) {
      std::cout << "Error getting client fd\n";
      continue;
      // error
    }

    Connection* conn = (Connection*) malloc(sizeof(Connection));
    conn->fd = clientfd;
    queue->push(conn);    
  }

  return (void *) 0;
};
