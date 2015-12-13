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

void handler(int sig) {
  running.unlock();
}

int main(int argc, char *argv[]) {
  log4cpp::Priority::PriorityLevel level = log4cpp::Priority::INFO;
  log4cpp::Category* log;
  try {
    log = &logging_init(level);
  } catch(log4cpp::ConfigureFailure e) {
    std::cout << "Fatal: " << e.what() << std::endl;
    return -1;
  }

  log->info("### STARTING ###");

  // use of streams for logging messages
  log->info("Version: %s", version.c_str());

  qttp = new QTTP(log);
  int result = qttp->Start(8080);
  if (result == -1) {
    // Error
    log->fatal("Error starting qttp");
    log->shutdown();

    return result;
  }

  // Signal handling
  struct sigaction sa = {0};
  sa.sa_handler = handler;
  sa.sa_flags = SA_RESTART;
  result = sigaction(SIGINT, &sa, NULL); 
  if (result == -1) {
    log->fatal("sigation erorr: ", strerror(errno));
    log->shutdown();

    return -1;
  } 

  // Block exit, sig handler will release first lock when ready to exit
  running.lock();
  running.lock();

  result = qttp->Stop();
  if (result == -1) {
    // Error
    log->fatal("Error stopping qttp");
    log->shutdown();

    return result;
  }

  delete qttp;

  log->info("### EXITING ###");

  log->shutdown();

  return 0;
}
