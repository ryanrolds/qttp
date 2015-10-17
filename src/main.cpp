#include "qttp.h"
#include "logging.h"

#include <cstddef>
#include <errno.h>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <string.h>

std::string version = "0.0.1";

std::mutex running;
QTTP *qttp;

void cleanup() {
  delete qttp;
};

void handler(int sig) {
  running.unlock();
}

int main(int argc, char *argv[]) {
  
  log4cpp::Priority::PriorityLevel level = log4cpp::Priority::DEBUG;
  log4cpp::Category& log = logging_init(level);

  // use of streams for logging messages
  log << log4cpp::Priority::INFO << "Version: " << version;

  qttp = new QTTP(&log);
  int result = qttp->Start(8080);
  if (result == -1) {
    // Error
    return result;
  }

  // Signal handling
  struct sigaction sa = {0};
  sa.sa_handler = handler;
  sa.sa_flags = SA_RESTART;
  result = sigaction(SIGINT, &sa, NULL); 
  if (result == -1) {
    std::cout << "sigaction error " << strerror(errno)  << "\n";
    cleanup();
    return -1;
  } 

  // Block exit, sig handler will release first lock when ready to exit
  running.lock();
  running.lock();

  result = qttp->Stop();
  if (result == -1) {
    // Error
    return result;
  }

  cleanup();

  return 0;
}
