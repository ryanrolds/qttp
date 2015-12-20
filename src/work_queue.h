#pragma once

#include "request.h"

#include <condition_variable>
#include <mutex>
#include <queue>

class WorkQueue {
 private:
  std::mutex mtx;
  std::condition_variable cv;
  std::queue<request*> queue;
  bool shutdown_queue;
  
 public:
  WorkQueue();
  ~WorkQueue();
  int push(request*);
  request* pop();
  int size();
  bool running();
  int shutdown();
};
