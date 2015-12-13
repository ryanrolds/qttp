#include "connection_handler_epoll.h"

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

ConnectionHandlerEpoll::ConnectionHandlerEpoll(log4cpp::Category *c, 
					       ConnectionPool *p,
					       ConnectionQueue *q) {
  log = c;
  pool = p;

  log->debug("Pool %d", pool);

  queue = q;
};

int ConnectionHandlerEpoll::Start(int socketfd) {
  listenfd = socketfd;

  log->debug("Pool %d", pool);  

  // Will use this to talk to the thread
  int result = pipe(connection_pipefd);
  if (result == -1) {
    log->fatal("Connection pipe create error: %s", strerror(errno));
    return -1;
  }

  log->debug("Connection pipe read fd %d", connection_pipefd[0]);
  log->debug("Connection pipe write fd %d", connection_pipefd[1]);

  noticefd = connection_pipefd[0];

  // Start thread
  log->debug("Starting connection handler thread");
  thread = std::thread(&ConnectionHandlerEpoll::Handler, this);
  log->debug("Connection handler thread started");
};

int ConnectionHandlerEpoll::Stop() {
  char s[] = "s";
  log->debug("Sending kill pill %d", connection_pipefd[1]);
  int result = write(connection_pipefd[1], (void *)&s, 1);
  if (result == -1) {
    log->fatal("Pipe write error: %s", strerror(errno));
  }

  thread.join();

  return 0;
};

ConnectionHandlerEpoll::~ConnectionHandlerEpoll() {};

void* ConnectionHandlerEpoll::Handler() {
  struct epoll_event ev = {0};
  struct epoll_event *events;

  events = (epoll_event*) calloc(MAXEVENTS, sizeof(epoll_event));

  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    log->error("epoll_create1 error: %s", strerror(errno));
  }

  log->debug("Thread starting");

  // Added notice/pipe fd to epoll set
  ev.events = EPOLLIN;
  ev.data.fd = noticefd;

  int epctl = epoll_ctl(epollfd, EPOLL_CTL_ADD, noticefd, &ev);
  if (epctl == -1) {
    log->fatal("epoll_ctl noticefd error: %s", strerror(errno));
    return (void*) -1;
  }

  // Add socketfd to epoll set
  ev.events = EPOLLIN;
  ev.data.fd = listenfd;

  epctl = epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
  if (epctl == -1) {
    log->fatal("epoll_ctl error: %s", strerror(errno));
    return (void*) -1;
  }

  // Update handler status
  status = HANDLER_RUNNING;

  int n;
  int result = 0;

  // Main fd event loop
  while (status != HANDLER_SHUTDOWN) {
    log->debug("tick");

    // Blocking epoll wait
    int nfds = epoll_wait(epollfd, events, MAXEVENTS, -1);
    // Check if no events
    if (nfds == 0) {
      log->debug("No events");
      continue; 
    }

    // Handle error
    if (nfds == -1) {
      log->fatal("epoll_wait error: %s", strerror(errno));
      return (void*) -1;
    }

    // Handle events one at a time
    for (n = 0; n < nfds; n++) {
      if (events[n].data.fd == listenfd) {
	log->debug("New connection");

	// New connection event
	result = handle_connection(&events[n]);
	if (result != 0) {
	  log->error("conn handler error: %s", result);
	}
      } else if (events[n].data.fd == noticefd) {
	log->debug("New notice");

	// Notice/pipe event
	result = handle_notice(&events[n]);
	if (result != 0) {
	  log->debug("notice handler error %s", result);
	}
      } else {
	log->debug("New data");

	// Connection data event
	result = handle_data(&events[n]);
	if (result != 0) {
	  log->error("data handler error: %s", result);
	}
      }
    }

    log->debug("tock");
  }

  free(events);

  log->debug("Thread stopped");

  return 0;
}

int ConnectionHandlerEpoll::handle_notice(struct epoll_event *ev) {
  const size_t bufferLen = 1;
  char buffer[bufferLen];
  
  ssize_t len = read(ev->data.fd, buffer, bufferLen);
  if (len < 0) {
    std::cout << "Probleming getting notice\n";
    std::cout << "recv notice error: " << strerror(errno) << "\n";
    return -1;
  }

  if (len == 0) {
    std::cout << "Missing notice?\n";
    return 0;
  }

  std::cout << "Got notice " << buffer[0] << "\n";
  
  if (buffer[0] == 's') {
    std::cout << "Connection shutdown initated!\n";

    on_shutdown();
  } else {
    std::cout << "Unknown notice " <<  buffer[0] << "\n";
  }
 
  return 0;
}

int ConnectionHandlerEpoll::handle_connection(struct epoll_event *ev) {
  log->debug("Inside handle connection");

  int connfd;
  struct sockaddr client = {0};
  socklen_t clientLen = sizeof(struct sockaddr);
  
  // Multiple connections can come in on a single EPOLLIN event
  // accept until we get EAGAIN
  while ((connfd = accept(ev->data.fd, &client, &clientLen)) > 0) {
    if (connfd == -1) {
      std::cout << "Error getting client fd\n";
      return -1;
    }

    log->debug("Connection accepted (%d)", connfd);

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

    int epctl = epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &clientev);
    if (epctl == -1) {
      std::cout << "epoll_ctl add error: " << strerror(errno) << "\n";
      return -1;
    }

    log->debug("Checkout connection object (%d)", connfd);
  
    log->debug("Checkout pool %d", pool);

    connections[connfd] = pool->Checkout(connfd);
  }

  if (connfd < 0 && errno != EAGAIN) {
      std::cout << "client accept error: " << strerror(errno) << "\n";
      return -1;
  }

  return 0;
}

void ConnectionHandlerEpoll::on_shutdown() {
  if (connection_count == 0) {
    status = HANDLER_SHUTDOWN;
    return;
  }

  status = HANDLER_DRAIN;
};

void ConnectionHandlerEpoll::on_connection() {
  connection_count++;
};

void ConnectionHandlerEpoll::on_disconnect() {
  connection_count--;

  if (status == HANDLER_DRAIN && connection_count == 0) {
    status= HANDLER_SHUTDOWN; 
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

int ConnectionHandlerEpoll::handle_data(epoll_event *ev) {
  size_t bufferLen = 2000;
  char buffer[bufferLen];
  ssize_t len;
  int clientfd = ev->data.fd;
  //int eventType = ev->events;
  
  connection *conn;
  try {
    conn = connections.at(clientfd);
  } catch (std::out_of_range e) {
    std::cout << "bad fd " << clientfd << "\n";
    throw e;
  }

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

    //std::cout << "recieved data " << len << "\n";

    if (conn->parser->upgrade) {
      // protocol renegotiation
    } else if (nparsed != len) {
      std::cout << "Protocol error\n";

      pool->Return(conn);
      return -1;
    }    
  }

  if (len == 0) { // FD closed
    if(conn->complete == 1) {
      std::cout << "What what\n";
    }

    /*
    std::cout << "Zero length message received\n";

    std::cout << "EPOLLIN: " << (EPOLLIN & eventType == EPOLLIN ? 't' : 'f') << "\n";
    std::cout << "EPOLLOUT: " << ((EPOLLOUT & eventType == EPOLLOUT) ? 't' : 'f') << "\n";
    std::cout << "EPOLLRDHUP: " << ((EPOLLRDHUP & eventType == EPOLLRDHUP) ? 't' : 'f') << "\n";
    std::cout << "EPOLLPRI: " << ((EPOLLPRI & eventType == EPOLLPRI) ? 't' : 'f') << "\n";
    std::cout << "EPOLLERR: " << ((EPOLLERR & eventType == EPOLLERR) ? 't' : 'f') << "\n";
    std::cout << "EPOLLHUP: " << ((EPOLLHUP & eventType == EPOLLHUP) ? 't' : 'f') << "\n";
    std::cout << "EPOLLET: " << ((EPOLLET & eventType == EPOLLET) ? 't' : 'f') << "\n";
    std::cout << "EPOLLONESHOT: " << ((EPOLLONESHOT & eventType == EPOLLONESHOT) ? 't' : 'f') << "\n";
    */
    
    //std::cout << "Size 0\n";
    pool->Return(conn);
    return 0;
  }

  if (len < 0) {
    if (errno != EAGAIN) {
      std::cout << "Recv error: " << strerror(errno) << "\n";

      pool->Return(conn);
      return -1;
    } else if (conn->complete != 1) {
      // Connection not finished, wait for more data
      return 0;
    }
  }

  if(conn->complete == 1) {
    int epctl = epoll_ctl(epollfd, EPOLL_CTL_DEL, conn->fd, NULL);
    if (epctl == -1) {
      std::cout << "epoll_ctl del error: " << strerror(errno) << "\n";
      return -1;
    }

    connections.erase(conn->fd);

    queue->push(conn);    
    return 0;
  }

  return 0;
}
