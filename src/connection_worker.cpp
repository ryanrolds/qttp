#include "connection_worker.h"

#include <iostream>
#include <string.h>
#include <sys/socket.h>

char response[] = "HTTP/1.1 200\r\nContent-Length: 4\r\nContent-Type: text/html\r\n\r\nblah";
int response_length = strlen(response);

void *connection_worker(int noticefd, int socketfd, ConnectionQueue *queue) {
  struct worker_state status;
  status.current_state = WORKER_START;

  while (status.current_state != WORKER_SHUTDOWN) {
    connection* conn = queue->pop();

    int sent = send(conn->fd, response, response_length, 0);
    if (sent < response_length) {
      std::cout << "Didn't send all\n";
    }

    if (sent < 0) {
      std::cout << "Send error: " << strerror(errno) << "\n";
    }

    destroy_connection(conn);
  }

  return 0;
};
