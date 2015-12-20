#include "connection_pool.h"
#include "connection_queue.h"
#include "work_queue.h"
#include "logging.h"

#include <map>

#include <sys/epoll.h>
#include <thread>

typedef enum {WORKER_STOPPED, WORKER_STARTED, WORKER_RUNNING, WORKER_DRAIN, WORKER_SHUTDOWN} worker_states;

const int MAXCONNEVENTS = 50;

class ConnectionWorker {
 private:
  log4cpp::Category* log;
  ConnectionPool* pool;
  ConnectionQueue* queue;
  WorkQueue* workQueue;

  std::thread thread;

  int connection_pipefd[2];
  int noticefd;
  int eventfd;
  int epollfd;

  int connection_count = 0;
  worker_states status = WORKER_STOPPED;
  std::map<int, connection*> connections;

  int handle_connection(connection*);
  int handle_notice(struct epoll_event*);
  int handle_data(struct epoll_event*);

  // Events, update state machine
  void on_shutdown();
  void on_connection();
  void on_disconnect();

 public:
  ConnectionWorker(log4cpp::Category*, ConnectionPool*, ConnectionQueue*, WorkQueue*);
  int Start();
  void* Handler();
  int Stop();
  ~ConnectionWorker();
};
