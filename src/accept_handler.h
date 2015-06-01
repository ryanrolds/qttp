
struct accept_handler_args {
  ConnectionQueue *queue;
  int *socketfd;
};

void *accept_handler(accept_handler_args *);
