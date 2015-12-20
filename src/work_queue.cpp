#include "work_queue.h"

#include <iostream>

WorkQueue::WorkQueue() {
  shutdown_queue = false;
};

WorkQueue::~WorkQueue() {

};

bool WorkQueue::running() {
  return shutdown_queue == false;
}

int WorkQueue::size() {
  std::unique_lock<std::mutex> lock(mtx);

  int size = queue.size();

  lock.unlock();

  return size;
}

int WorkQueue::shutdown() {
  shutdown_queue = true;

  cv.notify_all();
  return 0;
};

int WorkQueue::push(request* conn) {
  std::unique_lock<std::mutex> lock(mtx);

  queue.push(conn);
  int size = queue.size();

  lock.unlock();
  cv.notify_one();

  return size;
};

request* WorkQueue::pop() {
  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock, [this]{ return shutdown_queue == true || queue.empty() == false; });

  if (queue.empty() && shutdown_queue == true) {
    lock.unlock();
    return NULL;
  }

  request *conn = queue.front();
  queue.pop();

  lock.unlock();

  return conn;
};
