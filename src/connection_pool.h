#pragma once

#include "http_parser.h"

#include <mutex>
#include <condition_variable>
#include <stack> 

#include <map>
#include <string>
#include <vector>

struct connection {
  int fd;
  http_parser *parser;
  std::string url;
  std::string field;
  std::string value;
  std::map<std::string, std::string> headers;
  std::vector<char> body;
  int complete = 0;
  bool exit = false; // Poison pill
};

class ConnectionPool {
 private:
  int limit = 100;
  int counter = 0;
  std::stack<connection*> pool;
  std::mutex mtx;
  std::condition_variable cv;
  
 public:
  ConnectionPool();
  ConnectionPool(int);
  ~ConnectionPool();
  connection* aquire();
  int release(connection*);
  int getLimit();
  int getAvailable();
};

connection* create_connection(ConnectionPool*, int);
int destroy_connection(ConnectionPool*, connection*);
