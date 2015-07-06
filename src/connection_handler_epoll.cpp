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

void handler_accepting(handler_state *handler) {
  handler->current_state = READY;
};

void handler_shutdown(handler_state *handler) {
  if (handler->connection_count == 0) {
    handler->current_state = SHUTDOWN;
    return;
  }

  handler->current_state = DRAIN;
};

void handler_connection(handler_state *handler) {
  handler->connection_count++;
};

void handler_disconnection(handler_state *handler) {
  handler->connection_count--;

  if (handler->current_state == DRAIN && handler->connection_count == 0) {
    handler->current_state = SHUTDOWN; 
  }
};

void *connection_handler_epoll(int noticefd, int socketfd) {
  int n;
  int MAXEVENTS = 10;
  struct epoll_event ev = {0};
  struct epoll_event *events;

  struct handler_state handler;
  handler.current_state = START; 

  events = (epoll_event*) calloc(MAXEVENTS, sizeof(epoll_event));

  int epfd = epoll_create(100);
  if (epfd == -1) {
    std::cout << "epoll_create1 error: " << strerror(errno) << "\n";
  }

  std::cout << "Thread started\n";

  ev.events = EPOLLIN;
  ev.data.fd = noticefd;

  int epctl = epoll_ctl(epfd, EPOLL_CTL_ADD, noticefd, &ev);
  if (epctl == -1) {
    std::cout << "epoll_ctl noticefd error: " << strerror(errno) << "\n";
    return (void*) -1;
  }

  ev.events = EPOLLIN;
  ev.data.fd = socketfd;

  epctl = epoll_ctl(epfd, EPOLL_CTL_ADD, socketfd, &ev);
  if (epctl == -1) {
    std::cout << "epoll_ctl error: " << strerror(errno) << "\n";
    return (void*) -1;
  }

  handler_accepting(&handler);

  while (handler.current_state != SHUTDOWN) {
    int nfds = epoll_wait(epfd, events, MAXEVENTS, 1000);
    if (nfds == 0) {
      continue;
    }

    if (nfds == -1) {
      std::cout << "epoll_wait error: " << strerror(errno) << "\n";
      return (void*) -1;
    }

    for (n = 0; n < nfds; n++) {
      if (events[n].data.fd == socketfd) {
	handleConnection(&handler, epfd, &events[n]);
      } else if (events[n].data.fd == noticefd) {
	handleNotice(&handler, &events[n]);
      } else {
	handleData(&events[n]);
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
  struct sockaddr client = {0};
  socklen_t clientLen = sizeof(struct sockaddr);

  int connfd = accept(ev->data.fd, &client, &clientLen);
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

  connection *conn = new connection();
  conn->parser = (http_parser*) malloc(sizeof(http_parser));
  conn->fd = connfd;

  http_parser_init(conn->parser, HTTP_REQUEST);
  conn->parser->data = (void*) conn;
  
  connections[connfd] = conn;

  return 0;
}

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

int handleData(epoll_event *ev) {
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

  char response[] = "HTTP/1.1 200\r\nContent-Length: 4\r\nContent-Type: text/html\r\n\r\nblah";
  int response_length = strlen(response);

  ssize_t nparsed;

  while ((len = recv(clientfd, buffer, bufferLen, 0)) > 0) {
    nparsed = http_parser_execute(conn->parser, &settings, buffer, len);

    if (conn->parser->upgrade) {
      // protocol renegotiation
    } else if (nparsed != len) {
      std::cout << "Protocol error\n";

      close_connection(conn);
    }    
  }

  if (len < 0) {
    if (errno == EAGAIN) {
      if (conn->complete != 1) {
	// Connection not finished, wait for more data
	return 0;
      }
    } else {
      std::cout << "Recv error: " << strerror(errno) << "\n";

      close_connection(conn);
      return -1;
    }
  }

  //std::cout << "Sending response " << clientfd << "\n";

  int result = send(clientfd, response, response_length, 0);
  if (result < 0) {
    std::cout << "Send error: " << strerror(errno) << "\n";
  }

  result = close(clientfd);
  if (result != 0) {
    std::cout << "Close error: " << strerror(errno) << "\n";
  }

  close_connection(conn);
 
  return 0;
}

int close_connection(connection *conn) {
  connections.erase(conn->fd);
  free(conn->parser);
  delete conn;

  return 0;
}
