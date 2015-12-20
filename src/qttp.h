#include "connection_handler_epoll.h"
#include "connection_queue.h"
#include "connection_worker.h"
#include "connection_pool.h"
#include "logging.h"

#include <netdb.h>
#include <unistd.h>

const size_t NUM_WORKERS = 1;

class QTTP {
 private:
   ConnectionQueue *queue;
   WorkQueue* workQueue;
   ConnectionPool *pool;

   struct addrinfo hints; 
   int socketfd;
   ConnectionHandlerEpoll *connection_handler;
   std::array<ConnectionWorker*, NUM_WORKERS> workers;
   int worker_pipefd[2];
   int boundPort;

   log4cpp::Category *log;

 public:
   QTTP(log4cpp::Category *cat);
   // Primary functions
   int Start(int port);
   int Stop();

   // Should only be used for testing 
   int Bind(int port);
   int AcceptConnections();
   int StopConnections();
   int StartWorkers();
   int StopWorkers();
   int Listen();
   int StopListening();

   int GetPort();
   ~QTTP();
};
