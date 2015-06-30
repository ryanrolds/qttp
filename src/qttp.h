#include <netdb.h>
#include <thread>
#include <unistd.h>

const size_t NUM_WORKERS = 1;

class QTTP {
 private:
   struct addrinfo hints; 
   int socketfd;
   std::array<std::thread, NUM_WORKERS> workers;
   int pipefd[2];

 public:
   QTTP();
   int Bind();
   int StartWorkers();
   int StopWorkers();
   int Listen();
   ~QTTP();
};
