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
  int result = qttp->Bind(8080);
  if (result == -1) {
    std::cout << "Error binding\n";
    cleanup();
    return -1;
  }

  result = qttp->AcceptConnections();
  if (result == -1) {
    std::cout << "Error starting accept thread\n";
    cleanup();
    return -1;
  }

  result = qttp->StartWorkers();
  if (result == -1) {
    std::cout << "Error starting workers\n";
    cleanup();
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
    cleanup();
    return -1;
  } 

  // Block exit, sig handler will release first lock when ready to exit
  running.lock();
  running.lock();

  qttp->StopConnections();
  qttp->StopListening();
  qttp->StopWorkers();
  cleanup();

  return 0;
}
