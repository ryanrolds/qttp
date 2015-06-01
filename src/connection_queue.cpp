#include "connection_queue.h"

#include <iostream>
#include <thread>

ConnectionQueue::ConnectionQueue() {}

void ConnectionQueue::push(Connection *conn) {
  std::cout << std::this_thread::get_id() << " Calling push\n";

  std::unique_lock<std::mutex> lk(mtx);

  queue.push(conn);

  std::cout << std::this_thread::get_id() << " Pushed connection " << conn->fd << "" <<
    queue.size() << "\n";

  lk.unlock();

  cond.notify_one();
}

Connection* ConnectionQueue::pop() {
  std::cout << std::this_thread::get_id() << " Calling pop\n";

  std::unique_lock<std::mutex> lk(mtx);

  std::cout << std::this_thread::get_id() << " Got pop lock\n";

  cond.wait(lk, [this]{return (queue.empty() == false);});

  std::cout << std::this_thread::get_id() << " Pop post wait\n";

  Connection *conn = queue.top();
  queue.pop();

  std::cout << std::this_thread::get_id() <<  " Popped " << conn->fd << " " <<
    queue.size() << std::endl;

  lk.unlock();
  return conn;
}
