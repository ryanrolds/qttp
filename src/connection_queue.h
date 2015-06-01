#include <condition_variable>
#include <mutex>
#include <queue> 

struct Connection {
  int fd;
};

class ConnectionQueue {
private:
  std::mutex mtx;
  std::condition_variable cond;
  std::priority_queue<Connection *> queue;

public:
  ConnectionQueue();
  void push(Connection *);
  Connection* pop();
};

