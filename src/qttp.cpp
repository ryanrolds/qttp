#include "qttp.h"

#include <array>
#include <errno.h>
#include <iostream>

#include <string.h>
#include <sys/socket.h>

#include <fcntl.h>

QTTP::QTTP() {
 queue = new ConnectionQueue();
};

int QTTP::Bind() {
  // Addr hints, will look up interface
  hints = {0};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_flags = hints.ai_flags | AI_PASSIVE;
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

  int flags = fcntl(socketfd, F_GETFL);
  if (flags == -1) {
    std::cout << "socket getfl error: " << strerror(errno) << "\n";
    return -1;
  }

  flags |= O_NONBLOCK; // Set non-blocking

  int sfdctl = fcntl(socketfd, F_SETFL, flags);
  if (sfdctl == -1) {
    std::cout << "socket setfl error: " << strerror(errno) << "\n";
    return -1;
  }

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
  // net.core.somaxconn can limit backlog, this to SOMAXCONN
  listen(socketfd, 1024);

  return 0;
};

int QTTP::AcceptConnections() {
  int result = pipe(connection_pipefd);
  if (result == -1) {
    std::cout << "Connection pipe create error: " << strerror(errno) << "\n";
    return -1;
  }

  std::cout << "Connection pipe read fd " << connection_pipefd[0] << "\n";
  std::cout << "Connection pipe write fd " << connection_pipefd[1] << "\n";

  std::cout << "Starting connection handler\n";
  connection_thread = std::thread(connection_handler_epoll, connection_pipefd[0], socketfd, queue);

  return 0;
};

int QTTP::StopConnections() {
  return 0;
};

int QTTP::StartWorkers() {
  int result = pipe(worker_pipefd);
  if (result == -1) {
    std::cout << "Worker pipe create error: " << strerror(errno) << "\n";
    return -1;
  }

  std::cout << "Worker pipe read fd " << worker_pipefd[0] << "\n";
  std::cout << "Worker pipe write fd " << worker_pipefd[1] << "\n";
  
  // Create Worker threads, queue from connection queue
  for (size_t i = 0; i < NUM_WORKERS; i++) {
    std::cout << "Starting worker " << i << "\n";

    // Create worker that uses epoll connection handler
    workers[i] = std::thread(connection_worker, worker_pipefd[0], socketfd, queue);
  }

  return 0;
};

int QTTP::StopWorkers() {
  int result = close(socketfd);
  if (result != 0) {
    std::cout << "Close error: " << strerror(errno) << "\n";
  }

  char s[] = "s";

  // Send enough kill pills for each thread
  for (size_t i = 0; i < workers.size(); i++) {
    std::cout << "Sending kill pill " << worker_pipefd[1] << "\n";
    result = write(worker_pipefd[1], (void *)&s, 1);
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
