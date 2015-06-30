#include "connection_handler_epoll.h"
#include "connection_queue.h"
#include "http_parser.h"

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

http_parser *parser;

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

  parser = (http_parser*) malloc(sizeof(http_parser));

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
  free(parser);

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
  
  return 0;
}

int handleData(epoll_event *ev) {
  size_t bufferLen = 2000;
  char buffer[bufferLen];
  ssize_t len;
  int clientfd = ev->data.fd;
      
  http_parser_settings settings = {0};

  char response[] = "HTTP/1.1 200\r\nContent-Length: 4\r\nContent-Type: text/html\r\n\r\nblah";
  int response_length = strlen(response);

  http_parser_init(parser, HTTP_REQUEST);
  parser->data = (void*) &clientfd;

  ssize_t nparsed;

  while ((len = recv(clientfd, buffer, bufferLen, 0)) > 0) {
    nparsed = http_parser_execute(parser, &settings, buffer, len);

    if (parser->upgrade) {
      // protocol renegotiation
    } else if (nparsed != len) {
      // PROBLEM
    }
  }

  if (len < 0) {
    if (errno != EAGAIN) {
      std::cout << "Send error: " << strerror(errno) << "\n";
      return -1;
    }
  }

  int result = send(clientfd, response, response_length, 0);
  if (result < 0) {
    std::cout << "Send error: " << strerror(errno) << "\n";
  }

  result = close(clientfd);
  if (result != 0) {
    std::cout << "Close error: " << strerror(errno) << "\n";
  }
 
  return 0;
}
