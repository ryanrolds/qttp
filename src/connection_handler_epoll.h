#include "connection_queue.h"

typedef enum {HANDLER_START, HANDLER_READY, HANDLER_DRAIN, HANDLER_SHUTDOWN} handler_states;

struct handler_state {
  handler_states current_state;
  int connection_count = 0;
};

void handler_accepting(handler_state*);
void handler_shutdown(handler_state*);
void handler_connection(handler_state*);
void handler_disconnection(handler_state*);

void *connection_handler_epoll(int, int, ConnectionQueue*);
int handleConnection(handler_state*, int, struct epoll_event*);
int handleNotice(handler_state*, struct epoll_event*);
int handleData(struct epoll_event*, ConnectionQueue*);
