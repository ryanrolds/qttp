#include "connection_queue.h"

#include <iostream>

ConnectionQueue::ConnectionQueue() {
 
};

ConnectionQueue::~ConnectionQueue() {
  
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
  cv.wait(lock, [this]{ return queue.empty() == false; });
  
  //std::cout << "Connection pop\n";

  connection *conn = queue.front();
  queue.pop();

  lock.unlock();

  return conn;
};

