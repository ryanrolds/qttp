#include "connection_worker.h"

void *connection_worker(int noticefd, int socketfd, ConnectionQueue *queue) {
  struct worker_state status;
  status.current_state = WORKER_START;

  while (status.current_state != WORKER_SHUTDOWN) {
    
  }

  return 0;
};
