#include "connection_handler_epoll.h"
#include "connection_queue.h"
#include "connection_worker.h"
#include "connection_pool.h"

#include <netdb.h>
#include <thread>
#include <unistd.h>

const size_t NUM_WORKERS = 2;

class QTTP {
 private:
   struct addrinfo hints; 
   int socketfd;
   std::thread connection_thread;
   std::array<std::thread, NUM_WORKERS> workers;
   int connection_pipefd[2];
   int worker_pipefd[2];
   ConnectionQueue *queue;
   ConnectionPool *pool;

 public:
   QTTP();
   int Bind();
   int AcceptConnections();
   int StopConnections();
   int StartWorkers();
   int StopWorkers();
   int Listen();
   int StopListening();
   ~QTTP();
};
