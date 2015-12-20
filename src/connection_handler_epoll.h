#include "connection_pool.h"
#include "connection_queue.h"
#include "logging.h"

#include <sys/epoll.h>
#include <thread>

typedef enum {HANDLER_STOPPED, HANDLER_STARTED, HANDLER_RUNNING, HANDLER_DRAIN, HANDLER_SHUTDOWN} handler_states;

const int MAXEVENTS = 50;

class ConnectionHandlerEpoll {
 private:
  log4cpp::Category* log;
  ConnectionPool* pool;
  ConnectionQueue* queue;

  std::thread thread;

  int connection_pipefd[2];
  int noticefd; // Used to talk to thread
  int listenfd;
  int epollfd;

  int connection_count = 0;
  handler_states status = HANDLER_STOPPED;

  int handle_connection(struct epoll_event*);
  int handle_notice(struct epoll_event*);

  // Events, update state machine
  void on_shutdown();
  void on_connection();
  void on_disconnect();
  
 public:
  ConnectionHandlerEpoll(log4cpp::Category*, ConnectionPool*, ConnectionQueue*);
  int Start(int);
  void* Handler();
  int Stop();
  ~ConnectionHandlerEpoll();
};
