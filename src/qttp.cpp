#include "qttp.h"
#include "connection_handler_epoll.h"
//#include "connection_queue.h"
//#include "accept_handler.h"

#include <array>
#include <errno.h>
#include <iostream>


#include <string.h>
#include <sys/socket.h>

#include <fcntl.h>

QTTP::QTTP() {};

int QTTP::Bind() {
  hints = {0};
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
    free(address); // Valgrind clean
    std::cout << "getaddrinfo: " << strerror(errno) << "\n"; 
    return -1;
  }

  // Create socket 
  socketfd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);

  // Bind socket
  result = bind(socketfd, address->ai_addr, address->ai_addrlen); 
  if (result != 0) {
    free(address); // Valgrind clean
    std::cout << "Bind error: " << strerror(errno) << "\n";
    return -1;
  }

  // Done with address
  free(address);

  return 0;
};

int QTTP::Listen() {
 // Listen for connections
  listen(socketfd, 3);

  return 0;
}

int QTTP::StartWorkers() {
  int result = pipe(pipefd);
  if (result == -1) {
    std::cout << "Pipe create error: " << strerror(errno) << "\n";
    return -1;
  }

  std::cout << "Pipe read fd " << pipefd[0] << "\n";
  std::cout << "Pipe write fd " << pipefd[1] << "\n";

  // Create Worker threads, queue from connection queue
  for (size_t i = 0; i < NUM_WORKERS; i++) {
    std::cout << "Starting worker " << i << "\n";

    // Create worker that uses epoll connection handler
    workers[i] = std::thread(connection_handler_epoll, pipefd[0], socketfd);
  }

  return 0;
}

int QTTP::StopWorkers() {
  int result = close(socketfd);
  if (result != 0) {
    std::cout << "Close error: " << strerror(errno) << "\n";
  }

  char s[] = "s";

  // Send enough kill pills for each thread
  for (size_t i = 0; i < workers.size(); i++) {
    std::cout << "Sending kill pill " << pipefd[1] << "\n";
    result = write(pipefd[1], (void *)&s, 1);
    if (result == -1) {
      std::cout << "Pipe write error: " << strerror(errno) << "\n";
    }
  }

  // Wait for threads to join
  std::cout << "Waiting for workers to join\n";
  for (size_t i = 0; i < workers.size(); i++) {
    workers[i].join();
  }

  return 0;
};

QTTP::~QTTP() {}
