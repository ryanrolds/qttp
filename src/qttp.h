#include <netdb.h>
#include <thread>
#include <unistd.h>

const size_t NUM_WORKERS = 1;

class QTTP {
 private:
   struct addrinfo hints; 
   int socketfd;
   std::thread connection_thread;
   std::array<std::thread, NUM_WORKERS> workers;
   int connection_pipefd[2];
   int worker_pipefd[2];
   ConnectonQueue *queue;

 public:
   QTTP();
   int Bind();
   int StartWorkers();
   int StopWorkers();
   int Listen();
   ~QTTP();
};
