#pragma once

#include "connection.h"

#include <condition_variable>
#include <mutex>
#include <queue>

class ConnectionQueue {
  private:
  std::mutex mtx;
  std::condition_variable cv;
  std::queue<connection *> queue;
 
 public:
  ConnectionQueue();
  ~ConnectionQueue();
  int push(connection*);
  connection* pop();
};
