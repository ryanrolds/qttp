#include "connection_queue.h"
#include "http_parser.h"

#include <errno.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

void *connection_handler(ConnectionQueue *queue) {
  size_t bufferLen = 2000;
  char buffer[bufferLen];
  ssize_t len;

  char response[] = "HTTP/1.1 200\r\nContent-Lengthx: 0\r\nContent-Type: text/html\r\n\r\n";
  int response_length = strlen(response);

  std::cout << "Thread started\n";

  http_parser_settings settings;
  //settings.on_url = url_handler;
  //settings.on_header_field = header_handler;

  Connection *conn;
  while ((conn = queue->pop())) {
    std::cout << "Got connection from queue\n";

    http_parser *parser = (http_parser*)malloc(sizeof(http_parser));
    http_parser_init(parser, HTTP_REQUEST);
    parser->data = (void*) &conn->fd;

    int clientfd = conn->fd;
    ssize_t nparsed;

    std::cout << "Client " << clientfd << "\n";

    len = recv(clientfd, buffer, bufferLen, 0);
    if (len < 1) {
      std::cout << "Bad length " << len << " " << "\n";
    }

    nparsed = http_parser_execute(parser, &settings, buffer, len);
    std::cout << "Parsed " << nparsed << " " << len << "\n";

    if (parser->upgrade) {
      // protocol renegotiation
    } else if (nparsed != len) {
      // PROBLEM
    }

    std::cout << "Done\n";

    int result = send(clientfd, response, response_length, 0);
    if (result < 0) {
      	std::cout << "Send error: " << strerror(errno) << "\n";
    }

    result = close(clientfd);
    if (result != 0) {
      	std::cout << "Close error: " << strerror(errno) << "\n";
    }

    free(conn);

    std::cout << "Client diconnection\n";
  }

  std::cout << "Thread stopped\n";

  return 0;
};
