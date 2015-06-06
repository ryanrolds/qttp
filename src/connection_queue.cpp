#include "connection_queue.h"

#include <iostream>
#include <thread>

ConnectionQueue::ConnectionQueue() {
  startup();
}

void ConnectionQueue::startup() {
  popNull = false;
};

void ConnectionQueue::shutdown() {
  std::cout << "Shutdown";
  popNull = true;
  cond.notify_all();
};

bool ConnectionQueue::running() {
  return popNull == false;
};

void ConnectionQueue::push(Connection *conn) {
  std::unique_lock<std::mutex> lk(mtx);
  queue.push(conn);
  lk.unlock();

  cond.notify_one();
}

Connection* ConnectionQueue::pop() {
  std::unique_lock<std::mutex> lk(mtx);
  cond.wait(lk, [this]{return (queue.empty() == false) || popNull;});

  if (popNull) {
    std::cout << "Sending null\n";
    lk.unlock();
    return NULL;
  }

  Connection *conn = queue.top();
  queue.pop();

  lk.unlock();

  return conn;
}
