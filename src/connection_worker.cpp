#include "connection_worker.h"

#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

char response[] = "HTTP/1.1 200\r\nContent-Length: 4\r\nContent-Type: text/html\r\n\r\nblah";
int response_length = strlen(response);

void *connection_worker(log4cpp::Category *log, ConnectionPool *pool,
			ConnectionQueue *queue) {
  struct worker_state status;
  status.current_state = WORKER_START;

  while (status.current_state != WORKER_SHUTDOWN) {
    connection* conn = queue->pop();
    if (conn == NULL) {
      log->info("Got poison pill");
      break;
    }

    int sent = send(conn->fd, response, response_length, 0);
    if (sent < response_length) {
      log->debug("Didn't send all");
    }

    if (sent < 0) {
      log->error("Send error: %s", strerror(errno));
    }

    pool->Return(conn);
  }

  log->info("Worker exiting");

  return 0;
};
