#include "connection_handler_epoll.h"

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

std::map<int, connection*> connections;

//ConnectionPool *pool;

void *connection_handler_epoll(int noticefd, int socketfd, ConnectionQueue *queue) {
  struct handler_state handler;
  handler.current_state = HANDLER_START; 

  //pool = new ConnectionPool();

  int MAXEVENTS = 50;

  struct epoll_event ev = {0};
  struct epoll_event *events;

  events = (epoll_event*) calloc(MAXEVENTS, sizeof(epoll_event));

  int epfd = epoll_create1(0);
  if (epfd == -1) {
    std::cout << "epoll_create1 error: " << strerror(errno) << "\n";
  }

  std::cout << "Thread started\n";

  // Added notice/pipe fd to epoll set
  ev.events = EPOLLIN;
  ev.data.fd = noticefd;

  int epctl = epoll_ctl(epfd, EPOLL_CTL_ADD, noticefd, &ev);
  if (epctl == -1) {
    std::cout << "epoll_ctl noticefd error: " << strerror(errno) << "\n";
    return (void*) -1;
  }

  // Add socketfd to epoll set
  ev.events = EPOLLIN;
  ev.data.fd = socketfd;

  epctl = epoll_ctl(epfd, EPOLL_CTL_ADD, socketfd, &ev);
  if (epctl == -1) {
    std::cout << "epoll_ctl error: " << strerror(errno) << "\n";
    return (void*) -1;
  }

  // Update handler status
  handler_accepting(&handler);

  int n;
  int result = 0;

  // Main fd event loop
  while (handler.current_state != HANDLER_SHUTDOWN) {
    int nfds = epoll_wait(epfd, events, MAXEVENTS, -1);
    // Check if no events
    if (nfds == 0) {
      continue; 
    }

    // Handle error
    if (nfds == -1) {
      std::cout << "epoll_wait error: " << strerror(errno) << "\n";
      return (void*) -1;
    }

    // Handle events one at a time
    for (n = 0; n < nfds; n++) {
      if (events[n].data.fd == socketfd) {
	// New connection event
	result = handleConnection(&handler, epfd, &events[n]);
	if (result != 0) {
	  std::cout << "conn handler error" << result << "\n";
	}
      } else if (events[n].data.fd == noticefd) {
	// Notice/pipe event
	result = handleNotice(&handler, &events[n]);
	if (result != 0) {
	  std::cout << "notice handler error" << result << "\n";
	}
      } else {
	// Connection data event
	result = handleData(&events[n], queue);
	if (result != 0) {
	  std::cout << "data handler error" << result << "\n";
	}
      }
    }
  }

  free(events);

  std::cout << "Thread stopped\n";

  return 0;
}

int handleNotice(handler_state *handler, struct epoll_event *ev) {
  const size_t bufferLen = 1;
  char buffer[bufferLen];
  
  ssize_t len = read(ev->data.fd, buffer, bufferLen);
  if (len < 0) {
    std::cout << "Problening getting notice\n";
    std::cout << "recv notice error: " << strerror(errno) << "\n";
    return -1;
  }

  if (len == 0) {
    std::cout << "Missing notice?\n";
    return 0;
  }

  std::cout << "Got notice " << buffer << "\n";
  
  if (buffer[0] == 's') {
    std::cout << "SHUTDOWN initated!\n";

    handler_shutdown(handler);
  }
 
  return 0;
}

int handleConnection(handler_state *handler, int epfd, struct epoll_event *ev) {
  int connfd;
  struct sockaddr client = {0};
  socklen_t clientLen = sizeof(struct sockaddr);
  
  // Multiple connections can come in on a single EPOLLIN event
  // accept until we get EAGAIN
  while ((connfd = accept(ev->data.fd, &client, &clientLen)) > 0) {;
    if (connfd == -1) {
      std::cout << "Error getting client fd\n";
      return -1;
    }

    int flags = fcntl(connfd, F_GETFL);
    if (flags == -1) {
      std::cout << "client get epoll_ctl error: " << strerror(errno) << "\n";
      return -1;
    }

    flags |= O_NONBLOCK;

    int cfdctl = fcntl(connfd, F_SETFL, flags);
    if (cfdctl == -1) {
      std::cout << "client set epoll_ctl error: " << strerror(errno) << "\n";
      return -1;
    }

    struct epoll_event clientev = {0};
    clientev.events = EPOLLIN | EPOLLET;
    clientev.data.fd = connfd;

    int epctl = epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &clientev);
    if (epctl == -1) {
      std::cout << "epoll_ctl error: " << strerror(errno) << "\n";
      return -1;
    }
  
    connections[connfd] = create_connection(connfd);;
  }

  if (connfd < 0 && errno != EAGAIN) {
      std::cout << "client accept error: " << strerror(errno) << "\n";
      return -1;
  }

  return 0;
}

void handler_accepting(handler_state *handler) {
  handler->current_state = HANDLER_READY;
};

void handler_shutdown(handler_state *handler) {
  if (handler->connection_count == 0) {
    handler->current_state = HANDLER_SHUTDOWN;
    return;
  }

  handler->current_state = HANDLER_DRAIN;
};

void handler_connection(handler_state *handler) {
  handler->connection_count++;
};

void handler_disconnection(handler_state *handler) {
  handler->connection_count--;

  if (handler->current_state == HANDLER_DRAIN && handler->connection_count == 0) {
    handler->current_state = HANDLER_SHUTDOWN; 
  }
};

int parser_on_url(http_parser *parser, const char *at, size_t length) {
  connection *conn = (connection*) parser->data;
  conn->url.append(at, length);

  return 0;
}

int parser_on_field(http_parser *parser, const char *at, size_t length) {
  connection *conn = (connection*) parser->data;

  if (conn->value.size() > 0 && conn->field.size()) {
    conn->headers[conn->field] = conn->value;

    conn->field.clear();
    conn->value.clear();
  }

  conn->field.append(at, length);

  return 0;
}

int parser_on_value(http_parser *parser, const char *at, size_t length) {
  connection *conn = (connection*) parser->data;
  conn->value.append(at, length); 

  return 0;
}

int headers_complete(http_parser *parser) {
  //connection *conn = (connection*) parser->data;

  //std::map<std::string, std::string>::iterator it = conn->headers.begin();
  //for (;it != conn->headers.end(); it++) {
  //  std::cout << it->first << ": " << it->second << "\n"; 
  //}

  return 0;
}

int parser_on_body(http_parser *parser, const char *at, size_t length) {
  connection *conn = (connection*) parser->data;

  conn->body.insert(conn->body.end(), at, at + length);

  return 0;
}

int message_complete(http_parser *parser) {
  connection *conn = (connection*) parser->data;
  conn->complete = 1;

  //std::cout << "Message complete " << conn->body.size() << "\n";

  return 0;
}

int handleData(epoll_event *ev, ConnectionQueue *queue) {
  size_t bufferLen = 2000;
  char buffer[bufferLen];
  ssize_t len;
  int clientfd = ev->data.fd;
  
  connection *conn = connections[clientfd];

  http_parser_settings settings = {0};
  settings.on_url = parser_on_url;
  settings.on_header_field = parser_on_field;
  settings.on_header_value = parser_on_value;
  settings.on_headers_complete = headers_complete;
  settings.on_body = parser_on_body;
  settings.on_message_complete = message_complete;

  ssize_t nparsed;

  while ((len = recv(clientfd, buffer, bufferLen, 0)) > 0) {
    nparsed = http_parser_execute(conn->parser, &settings, buffer, len);

    if (conn->parser->upgrade) {
      // protocol renegotiation
    } else if (nparsed != len) {
      std::cout << "Protocol error\n";

      destroy_connection(conn);
      return -1;
    }    
  }

  if (len == 0) {
    if(conn->complete == 1) {
      std::cout << "What what\n";
    }

    std::cout << "Zero length message received\n";
    
    //std::cout << "Size 0\n";
    destroy_connection(conn);
    return 0;
  }

  if (len < 0) {
    if (errno != EAGAIN) {
      std::cout << "Recv error: " << strerror(errno) << "\n";

      destroy_connection(conn);
      return -1;
    } else if (conn->complete != 1) {
      // Connection not finished, wait for more data
      return 0;
    }
  }

  if(conn->complete != 1) {
    queue->push(conn);
    connections.erase(conn->fd);
    return 0;
  }

  return 0;
}
