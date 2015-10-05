#include "connection_queue.h"

#include <iostream>

ConnectionQueue::ConnectionQueue() {
  shutdown_queue = false;
};

ConnectionQueue::~ConnectionQueue() {

};

bool ConnectionQueue::running() {
  return shutdown_queue == false;
}

int ConnectionQueue::size() {
  std::unique_lock<std::mutex> lock(mtx);

  int size = queue.size();

  lock.unlock();

  return size;
}

int ConnectionQueue::shutdown() {
  std::cout << "Queue shutting down\n";
  shutdown_queue = true;

  cv.notify_all();
  return 0;
};

int ConnectionQueue::push(connection* conn) {
  std::unique_lock<std::mutex> lock(mtx);

  //std::cout << "Push connection\n";

  queue.push(conn);
  int size = queue.size();

  lock.unlock();
  cv.notify_one();

  return size;
};

connection* ConnectionQueue::pop() {
  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock, [this]{ return shutdown_queue == true || queue.empty() == false; });

  if (queue.empty() && shutdown_queue == true) {
    std::cout << "Sending poison pill\n";

    lock.unlock();
    return NULL;
  }

  //std::cout << "Connection pop\n";

  connection *conn = queue.front();
  queue.pop();

  lock.unlock();

  return conn;
};
