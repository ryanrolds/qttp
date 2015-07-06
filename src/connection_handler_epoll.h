#include "http_parser.h"

#include <vector>
#include <string>
#include <map>

typedef enum {START, READY, DRAIN, SHUTDOWN} handler_states;

struct handler_state {
  handler_states current_state;
  int connection_count = 0;
};

struct connection {
  int fd;
  http_parser *parser;
  std::string url;
  std::string field;
  std::string value;
  std::map<std::string, std::string> headers;
  std::vector<char> body;
  int complete = 0;
};

void handler_accepting(handler_state*);
void handler_shutdown(handler_state*);
void handler_connection(handler_state*);
void handler_disconnection(handler_state*);

void *connection_handler_epoll(int, int);
int handleConnection(handler_state*, int, struct epoll_event*);
int handleNotice(handler_state*, struct epoll_event*);
int handleData(struct epoll_event*);

int close_connection(connection*);
