#include "qttp.h"

#include <array>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>

QTTP::QTTP(log4cpp::Category *cat) {
  log = cat;
  queue = new ConnectionQueue();
  workQueue = new WorkQueue();
  pool = new ConnectionPool(cat);
  connection_handler = new ConnectionHandlerEpoll(log, pool, queue);
};

int QTTP::Start(int port) {
  int result = Bind(port);
  if (result == -1) {
    return -1;
  }

  // Listen for connections
  result = Listen();
  if (result == -1) {
    return -1;
  }

  // Start connection handler
  result = connection_handler->Start(socketfd);
  if (result == -1) {
    return -1;
  }

  // Start connection workers
  result = StartWorkers();
  if (result == -1) {
    return -1;
  }

  return 0;
};

int QTTP::Stop() {
  connection_handler->Stop();
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
      log->fatal("getaddrinfo: %s", strerror(errno));
      return -1;
    }
  } else { // Should only be used for testing
    result = getaddrinfo("127.0.0.1", NULL, &hints, &address);
    if (result != 0) {
      free(address); // Valgrind clean
      log->fatal("getaddrinfo: %s", strerror(errno));
      return -1;
    }
  }

  // Create socket
  socketfd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);

  int flags = fcntl(socketfd, F_GETFL);
  if (flags == -1) {
    log->fatal("socket getfl error: %s",strerror(errno));
    return -1;
  }

  flags |= O_NONBLOCK; // Set non-blocking

  int sfdctl = fcntl(socketfd, F_SETFL, flags);
  if (sfdctl == -1) {
    log->fatal("socket setfl error: %s", strerror(errno));
      return -1;
  }

  // Bind socket
  result = bind(socketfd, address->ai_addr, address->ai_addrlen);
  if (result != 0) {
    free(address); // Valgrind clean
    log->fatal("Bind error: %s", strerror(errno));
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
    log->fatal("Sock name error: %s", strerror(errno));
    return -1;
  }

  boundPort = ntohs(address.sin_port);
  log->info("Lisenting on %d", boundPort);

  return 0;
};

int QTTP::StopListening() {
  int result;

  result = shutdown(socketfd, SHUT_RDWR);
  if (result != 0) {
    log->fatal("Shutdown error: %s", strerror(errno));
  }

  result = close(socketfd);
  if (result != 0) {
    log->fatal("Close error: %s", strerror(errno));
  }

  return 0;
}

int QTTP::StartWorkers() {
  // Create Worker threads, queue from connection queue
  for (size_t i = 0; i < NUM_WORKERS; i++) {
    log->debug("Starting worker %d", i);

    // Create worker that uses epoll connection handler
    workers[i] = new ConnectionWorker(log, pool, queue, workQueue);
    workers[i]->Start();
  }

  return 0;
};

int QTTP::StopWorkers() {
  queue->shutdown();

  // Wait for threads to join
  log->debug("Waiting for workers to join");
  for (size_t i = 0; i < workers.size(); i++) {
    workers[i]->Stop();
  }

  return 0;
};

int QTTP::GetPort() {
  return boundPort;
};

QTTP::~QTTP() {}
