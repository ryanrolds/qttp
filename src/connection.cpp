#include "connection.h"

#include <iostream>
#include <string.h>
#include <unistd.h>

connection* create_connection(int connfd) {
    connection *conn = new connection();
    conn->parser = (http_parser*) malloc(sizeof(http_parser));
    conn->fd = connfd;

    http_parser_init(conn->parser, HTTP_REQUEST);
    conn->parser->data = (void*) conn;

    return conn;
}

int destroy_connection(connection *conn) {
  if (close(conn->fd) != 0) {
    std::cout << "Close error: " << strerror(errno) << "\n";
  }

  //connections.erase(conn->fd);
  free(conn->parser);
  delete conn;

  return 0;
}
