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

Connection* ConnectionQueue::pop_nb() {
  std::unique_lock<std::mutex> lk(mtx);

  Connection *conn = _pop(); 

  lk.unlock();
  return conn;
}

Connection* ConnectionQueue::pop() {
  std::unique_lock<std::mutex> lk(mtx);
  cond.wait(lk, [this]{return (queue.empty() == false) || popNull;});

  Connection *conn = _pop();

  lk.unlock();
  return conn;
}

Connection* ConnectionQueue::_pop() {
  if (popNull) {
    std::cout << "Sending null\n";
    return NULL;
  }

  if (queue.empty()) {
    return NULL;
  }

  Connection *conn = queue.top();
  queue.pop();
  return conn;
}
