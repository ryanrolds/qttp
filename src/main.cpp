#include "qttp.h"

#include <cstddef>
#include <errno.h>
#include <iostream>
#include <mutex>
#include <signal.h>
#include <string.h>

std::mutex running;
QTTP *qttp;

void handler(int sig) {
  running.unlock();
}

int main(int argc, char *argv[]) {
  qttp = new QTTP();
  int result = qttp->Bind();
  if (result == -1) {
    std::cout << "Error binding\n";
    return -1;
  }

  result = qttp->StartWorkers();
  if (result == -1) {
    std::cout << "Error starting workers\n";
    return -1;
  }

  qttp->Listen();

  // @TODO Signal handling
  struct sigaction sa = {0};
  sa.sa_handler = handler;
  sa.sa_flags = SA_RESTART;
  result = sigaction(SIGINT, &sa, NULL); 
  if (result == -1) {
    std::cout << "sigaction error " << strerror(errno)  << "\n";
    return -1;
  } 

  running.lock();
  running.lock();

  qttp->StopWorkers();
  delete qttp;

  return 0;
}

