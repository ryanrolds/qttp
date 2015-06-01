#include "connection_queue.h"
#include "accept_handler.h"

#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>

void *accept_handler(accept_handler_args *args) {
  ConnectionQueue *queue = args->queue;

  struct sockaddr client = {0};
  socklen_t clientLen = sizeof(struct sockaddr);;
  int clientfd;

  std::cout << std::this_thread::get_id() << " Accept thread started\n";

  while((clientfd = accept(*args->socketfd, &client, &clientLen)) != -1) {
    std::cout << std::this_thread::get_id() << " Client connection\n";
    
    Connection* conn = (Connection*) malloc(sizeof(Connection));
    conn->fd = clientfd;
    queue->push(conn);

    std::cout << std::this_thread::get_id() << " Connection queued\n";
  }
  
  if (clientfd == -1) {
    std::cout << std::this_thread::get_id() << " Accept error: " << strerror(errno) << "\n";
    return (void *) 1;
  }

  std::cout << std::this_thread::get_id() << " Accept thread ended\n";

  return (void *) 0;
};
