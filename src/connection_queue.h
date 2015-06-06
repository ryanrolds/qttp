#include <condition_variable>
#include <mutex>
#include <queue> 

struct Connection {
  int fd;
};

class ConnectionQueue {
private:
  bool popNull;
  std::mutex mtx;
  std::condition_variable cond;
  std::priority_queue<Connection *> queue;

public:
  ConnectionQueue();
  void startup();
  void shutdown();
  bool running();
  void push(Connection *);
  Connection* pop();
};

