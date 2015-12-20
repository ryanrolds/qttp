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
  queue = q;
};

int ConnectionHandlerEpoll::Start(int socketfd) {
  listenfd = socketfd;

  log->debug("Listen for connections on fd (%d)", listenfd);

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
  log->info("Starting connection handler thread");
  thread = std::thread(&ConnectionHandlerEpoll::Handler, this);
  log->info("Connection handler thread started");
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
  log->debug("Accept thread starting");

  struct epoll_event ev = {0};
  struct epoll_event *events;

  events = (epoll_event*) calloc(MAXEVENTS, sizeof(epoll_event));

  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    log->error("epoll_create1 error: %s", strerror(errno));
  }

  // Added notice/pipe fd to epoll set
  ev.events = EPOLLIN;
  ev.data.fd = noticefd;

  log->debug("Accept adding notice fd (%d) to epoll", noticefd);

  int epctl = epoll_ctl(epollfd, EPOLL_CTL_ADD, noticefd, &ev);
  if (epctl == -1) {
    log->fatal("epoll_ctl noticefd error: %s", strerror(errno));
    return (void*) -1;
  }

  // Add socketfd to epoll set
  ev.events = EPOLLIN;
  ev.data.fd = listenfd;

  log->debug("Accept adding listen fd (%d) to epoll", listenfd);

  epctl = epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
  if (epctl == -1) {
    log->fatal("epoll_ctl listenfd error: %s", strerror(errno));
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

    log->debug("Found %d epoll events", nfds);

    // Handle events one at a time
    for (n = 0; n < nfds; n++) {
      log->debug("Event on fd (%d)", events[n].data.fd); 
      if (events[n].data.fd == listenfd) {
	log->debug("New connection");

	// New connection event
	result = handle_connection(&events[n]);
	if (result != 0) {
	  log->error("conn handler error: %d", result);
	}
      } else if (events[n].data.fd == noticefd) {
	log->debug("New notice");

	// Notice/pipe event
	result = handle_notice(&events[n]);
	if (result != 0) {
	  log->debug("notice handler error %s", result);
	}
      } else {
	log->warn("Unknown event in connection handler");
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
    log->error("Probleming getting notice");
    log->error("recv notice error: %s ", strerror(errno));
    return -1;
  }

  if (len == 0) {
    log->warn("Missing notice (%d)?", ev->data.fd);
    return 0;
  }

  log->debug("Got notice %c", buffer[0]);
  
  if (buffer[0] == 's') {
    log->debug("Connection shutdown initated!");

    on_shutdown();
  } else {
    log->warn("Unknown notice: %c", buffer[0]);
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
      log->error("Error getting client fd (%d)", ev->data.fd);
      return -1;
    }

    log->debug("Connection accepted (%d)", connfd);

    log->debug("Checkout connection object (%d)", connfd);
    connection* conn = pool->Checkout(connfd);

    log->debug("Adding connection (%d) to queue", connfd);
    queue->push(conn);
  }

  if (connfd < 0 && errno != EAGAIN) {
    log->error("client accept (%d) error: %s", ev->data.fd, strerror(errno));
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


