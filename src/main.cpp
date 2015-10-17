#include "qttp.h"

#include <cstddef>
#include <errno.h>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <string.h>

std::mutex running;
QTTP *qttp;

void cleanup() {
  delete qttp;
};

void handler(int sig) {
  running.unlock();
}

int main(int argc, char *argv[]) {
  qttp = new QTTP();
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
