#include "connection_queue.h"
#include "accept_handler.h"

#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <thread>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void *connection_handler(ConnectionQueue *);

int qttp(const int NUM_WORKERS) {
  // Setup hints needed by getaddrinfo
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // Address to listen for TCP conections on
  const char* port = "8080";
  struct addrinfo *address;
  int result = getaddrinfo(NULL, port, &hints, &address);
  if (result != 0) {
    std::cout << "getaddrinfo: " << strerror(errno) << "\n"; 
    return 1;
  }

  // Create socket 
  int socketfd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
  std::cout << "Socket fd: " << socketfd << "\n";

  // Bind socket
  result = bind(socketfd, address->ai_addr, address->ai_addrlen); 
  if (result != 0) {
    std::cout << "Bind error: " << strerror(errno) << "\n";
    return 1;
  }

  // Listen for connections
  listen(socketfd, 3);

  ConnectionQueue *queue = new ConnectionQueue();

  // Create thread that accepts connections
  accept_handler_args args;
  args.queue = queue;
  args.socketfd = &socketfd;

  std::cout << "Starting accept thread\n";
  std::thread accept_thread(accept_handler, &args);

  std::thread workers[NUM_WORKERS];
  // Create Worker threads, queue from connection queue
  for (int i = 0; i < NUM_WORKERS; i++) {
    std::cout << "Starting worker " << i << "\n";
    workers[i] = std::thread(connection_handler, queue);
  }

  // Process commands from TTY
  std::string word;
  while (std::cin >> word) {
    if (word.compare("exit") == 0) {
      break;
    }

    std::cout << word << "\n";
  }

  result = close(socketfd);
  if (result != 0) {
    std::cout << "Close error: " << strerror(errno) << "\n";
  }

  // TODO
  // join accept and all worker threads
  // clean up sockets

  return 0;
}
