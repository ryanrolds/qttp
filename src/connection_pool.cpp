#include "connection_pool.h"

#include <iostream>
#include <string.h>
#include <unistd.h>

ConnectionPool::ConnectionPool(log4cpp::Category *cat) {
  log = cat;
}
ConnectionPool::ConnectionPool(log4cpp::Category *cat, int max) {
  log = cat;
  limit = max;
};
ConnectionPool::~ConnectionPool() {};

connection* ConnectionPool::Checkout(int connfd) {
  log->debug("Inside connection object checkout (%d)", connfd);

  connection *conn = aquire();

  log->debug("Connection object aquired (%d)", connfd);

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
};

int ConnectionPool::Return(connection *conn) {
  log->debug("Return: preclose");
  if (close(conn->fd) != 0) {
    log->error("Close: %s",  strerror(errno));
  }
  
  log->debug("Return: pre-relese");

  int result = release(conn);
  if (result != 0) {
    log->debug("Return: release error");
    return -1;
  }

  log->debug("Return: post-release");

  return 0;
};

int ConnectionPool::GetLimit() {
  return limit;
};

int ConnectionPool::GetAvailable() {
  log->debug("available; get lock");
  std::unique_lock<std::mutex> lock(mtx);
  log->debug("available: got lock");

  int available = pool.size();

  lock.unlock();
  log->debug("available: release lock");

  return available;
};

connection* ConnectionPool::aquire() {
  log->debug("aquire: get lock");

  std::unique_lock<std::mutex> lock(mtx);
  log->debug("aquire: got lock");

  cv.wait(lock, [this]{ return pool.empty() != true || counter < limit; });
  log->debug("aquire: inside cv"); 

  connection *conn;
  if (pool.empty() == true) {
    conn = new connection();
    conn->parser = (http_parser*) malloc(sizeof(http_parser));
    counter++;
  } else {
    conn = pool.top();
    pool.pop();
  }
  
  lock.unlock();
  log->debug("aquire: release lock"); 
  
  return conn;
};

int ConnectionPool::release(connection* conn) {
  log->debug("release: get lock");
  std::unique_lock<std::mutex> lock(mtx);
  log->debug("release: got lock");
  
  pool.push(conn);
  
  lock.unlock();
  log->warn("release: release lock");
  cv.notify_all();
  log->warn("release: notify all");

  return 0;
};
