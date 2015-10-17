#include "qttp.h"

#include <array>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>

QTTP::QTTP(log4cpp::Category *cat) {
  log = cat;
  queue = new ConnectionQueue();
  pool = new ConnectionPool();
};

int QTTP::Start(int port) {
  int result = Bind(port);
  if (result == -1) {
    std::cout << "Error binding\n";
    return -1;
  }

  result = AcceptConnections();
  if (result == -1) {
    std::cout << "Error starting accept thread\n";
    return -1;
  }

  result = StartWorkers();
  if (result == -1) {
    std::cout << "Error starting workers\n";
    return -1;
  }

  Listen();

  return 0;
};

int QTTP::Stop() {
  StopConnections();
  StopListening();
  StopWorkers();
  return 0;
};

int QTTP::Bind(int port) {
  // Addr hints, will look up interface
  hints = {0};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  //hints.ai_canonname = NULL;
  //hints.ai_addr = NULL;
  hints.ai_flags = hints.ai_flags | AI_PASSIVE;
  //hints.ai_next = NULL;

  int result;

  // Address to listen for TCP conections on
  struct addrinfo *address;

  // Convert portfrom int to c string
  std::string port_str = std::to_string(port);
  const char* port_cstr = port_str.c_str();

  if (port != 0) {
    result = getaddrinfo(NULL, port_cstr, &hints, &address);
    if (result != 0) {
      free(address); // Valgrind clean
      std::cout << "getaddrinfo: " << strerror(errno) << "\n";
      return -1;
    }
  } else { // Should only be used for testing
    result = getaddrinfo("127.0.0.1", NULL, &hints, &address);
    if (result != 0) {
      free(address); // Valgrind clean
      std::cout << "getaddrinfo: " << strerror(errno) << "\n";
      return -1;
    }
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

  socklen_t address_len = sizeof(struct sockaddr_in);
  struct sockaddr_in address;

  // Get address/name of the socket
  int result = getsockname(socketfd, (struct sockaddr *) &address, &address_len);
  if (result < 0) {
    std::cout << "Sock name error: " << strerror(errno) << "\n";
    return -1;
  }

  boundPort = ntohs(address.sin_port);
  std::cout << "Port: " << boundPort << "\n";

  return 0;
};

int QTTP::StopListening() {
  int result;

  result = shutdown(socketfd, SHUT_RDWR);
  if (result != 0) {
    std::cout << "Shutdown error: " << strerror(errno) << "\n";
  }

  result = close(socketfd);
  if (result != 0) {
    std::cout << "Close error: " << strerror(errno) << "\n";
  }

  return 0;
}

int QTTP::AcceptConnections() {
  int result = pipe(connection_pipefd);
  if (result == -1) {
    std::cout << "Connection pipe create error: " << strerror(errno) << "\n";
    return -1;
  }

  std::cout << "Connection pipe read fd " << connection_pipefd[0] << "\n";
  std::cout << "Connection pipe write fd " << connection_pipefd[1] << "\n";

  std::cout << "Starting connection handler\n";
  connection_thread = std::thread(connection_handler_epoll, connection_pipefd[0], socketfd,
				  pool, queue);

  return 0;
};

int QTTP::StopConnections() {
  char s[] = "s";
  std::cout << "Sending kill pill " << connection_pipefd[1] << "\n";
  int result = write(connection_pipefd[1], (void *)&s, 1);
  if (result == -1) {
    std::cout << "Pipe write error: " << strerror(errno) << "\n";
  }

  connection_thread.join();

  return 0;
};

int QTTP::StartWorkers() {
  // Create Worker threads, queue from connection queue
  for (size_t i = 0; i < NUM_WORKERS; i++) {
    std::cout << "Starting worker " << i << "\n";

    // Create worker that uses epoll connection handler
    workers[i] = std::thread(connection_worker, pool, queue);
  }

  return 0;
};

int QTTP::StopWorkers() {
  queue->shutdown();

  // Wait for threads to join
  std::cout << "Waiting for workers to join\n";
  for (size_t i = 0; i < workers.size(); i++) {
    workers[i].join();
  }

  return 0;
};

int QTTP::GetPort() {
  return boundPort;
};

QTTP::~QTTP() {}
