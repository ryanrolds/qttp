#include "connection_queue.h"

typedef enum {WORKER_START, WORKER_READY, WORKER_DRAIN, WORKER_SHUTDOWN} worker_states;

struct worker_state {
  worker_states current_state;
};

void *connection_worker(ConnectionPool*, ConnectionQueue*);
