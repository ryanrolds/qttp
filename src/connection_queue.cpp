#include "connection_queue.h"

ConnectionQueue::ConnectionQueue() {
 
};

ConnectionQueue::~ConnectionQueue() {
  
};

int ConnectionQueue::push(connection* conn) {
  std::unique_lock<std::mutex> lock(mtx);
  lock.lock();

  queue.push(conn);
  int size = queue.size();

  lock.unlock();
  cv.notify_one();

  return size;
};

connection* ConnectionQueue::pop() { 
  std::unique_lock<std::mutex> lock(mtx);
  cv.wait(lock, [this]{ return queue.empty() == false; });
  
  connection *conn = queue.front();
  queue.pop();

  lock.unlock();

  return conn;
};

