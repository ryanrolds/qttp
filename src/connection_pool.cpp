#include "connection_pool.h"

#include <iostream>
#include <string.h>
#include <unistd.h>

ConnectionPool::ConnectionPool() {}
ConnectionPool::ConnectionPool(int max) {
  limit = max;
};
ConnectionPool::~ConnectionPool() {};

connection* ConnectionPool::aquire() {
  std::unique_lock<std::mutex> lock(mtx);
 
  connection *conn;
  if (pool.empty() == true) {
    conn = new connection();
    conn->parser = (http_parser*) malloc(sizeof(http_parser));
  } else {
    conn = pool.top();
    pool.pop();
  }
  
  lock.unlock();
  
  return conn;
};

int ConnectionPool::release(connection* conn) {
  std::unique_lock<std::mutex> lock(mtx);
  
  pool.push(conn);
  
  lock.unlock();
  return 0;
};

connection* create_connection(ConnectionPool *pool, int connfd) {
  connection *conn = pool->aquire();

  conn->fd = connfd;
  http_parser_init(conn->parser, HTTP_REQUEST);
  conn->parser->data = (void*) conn;

  conn->url.erase();
  conn->field.erase();
  conn->value.erase();
  conn->headers.clear();
  conn->body.clear();

  conn->complete = 0;
  conn->exit = false;
   
  return conn;
}

int destroy_connection(ConnectionPool *pool, connection *conn) {
  if (close(conn->fd) != 0) {
    std::cout << "Close error: " << strerror(errno) << "\n";
  }
  
  //connections.erase(conn->fd);
  //free(conn->parser);
  //delete conn;

  pool->release(conn);

  return 0;
}