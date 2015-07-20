
typedef enum {START, READY, DRAIN, SHUTDOWN} worker_states;

struct worker_state {
  worker_states current_state;
};

void *connection_worker(worker_state);
