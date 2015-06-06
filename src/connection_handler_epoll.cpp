#include "connection_queue.h"
#include "http_parser.h"

#include <errno.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

void *connection_handler_epoll(ConnectionQueue *queue) {
  size_t bufferLen = 2000;
  char buffer[bufferLen];
  ssize_t len;
  bool running = true;

  http_parser_settings settings = {0};
  http_parser *parser = (http_parser*)malloc(sizeof(http_parser));

  char response[] = "HTTP/1.1 200\r\nContent-Length: 4\r\nContent-Type: text/html\r\n\r\nblah";
  int response_length = strlen(response);

  std::cout << "Thread started\n";

  Connection *conn;
  while (running) {
    conn = queue->pop_nb();    
    if (conn == NULL) {
      // Add connection to epoll
      continue;
    }

    http_parser_init(parser, HTTP_REQUEST);
    parser->data = (void*) &conn->fd;

    int clientfd = conn->fd;
    ssize_t nparsed;

    len = recv(clientfd, buffer, bufferLen, 0);
    if (len < 1) {
      std::cout << "Bad length " << len << "\n";
    }

    nparsed = http_parser_execute(parser, &settings, buffer, len);
    //std::cout << "Parsed " << nparsed << " " << len << "\n";

    if (parser->upgrade) {
      // protocol renegotiation
    } else if (nparsed != len) {
      // PROBLEM
    }

    std::cout << "Responding to connection\n";

    int result = send(clientfd, response, response_length, 0);
    if (result < 0) {
      	std::cout << "Send error: " << strerror(errno) << "\n";
    }

    result = close(clientfd);
    if (result != 0) {
      	std::cout << "Close error: " << strerror(errno) << "\n";
    }

    free(conn);
  }

  std::cout << "Thread stopped\n";

  return 0;
};
