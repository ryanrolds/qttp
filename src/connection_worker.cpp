#include "connection_worker.h"

#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

char response[] = "HTTP/1.1 200\r\nContent-Length: 4\r\nContent-Type: text/html\r\n\r\nblah";
int response_length = strlen(response);

int parser_on_url(http_parser* parser, const char* at, size_t length) {
  // Build header's value
  ((connection*) parser->data)->url.append(at, length);

  return 0;
}

int parser_on_field(http_parser* parser, const char* at, size_t length) {
  connection* conn = (connection*) parser->data;

  // if value and field has data, then this must be start of new header record, so 
  // store existing header and prepare for next field+value pair
  if (conn->value.size() > 0 && conn->field.size()) {
    // Store current field+value pair in headers
    conn->headers[conn->field] = conn->value;

    // Prepare field and value for next header record
    conn->field.clear();
    conn->value.clear();
  }

  // Build header's field
  conn->field.append(at, length);

  return 0;
}

int parser_on_value(http_parser* parser, const char* at, size_t length) {
  connection* conn = (connection*) parser->data;
  conn->value.append(at, length); 

  return 0;
}

int headers_complete(http_parser* parser) {
  return 0;
}

int parser_on_body(http_parser* parser, const char* at, size_t length) {
  connection* conn = (connection*) parser->data;

  conn->body.insert(conn->body.end(), at, at + length);

  return 0;
}

int message_complete(http_parser* parser) {
  connection* conn = (connection*) parser->data;
  conn->complete = 1;

  conn->log->debug("Message complete %d", conn->body.size());

  return 0;
}

ConnectionWorker::ConnectionWorker(log4cpp::Category* c, ConnectionPool* p, ConnectionQueue* q, WorkQueue* w) {
  log = c;
  pool = p;
  queue = q;
  workQueue = w;
};

int ConnectionWorker::Start() {
  // Will use this to talk to the thread
  int result = pipe(connection_pipefd);
  if (result == -1) {
    log->fatal("Worker pipe create error: %s", strerror(errno));
    return -1;
  }

  log->debug("Worker pipe read fd %d", connection_pipefd[0]);
  log->debug("Worker pipe write fd %d", connection_pipefd[1]);

  noticefd = connection_pipefd[0];

  log->info("Starting connection worker thread");
  thread = std::thread(&ConnectionWorker::Handler, this);
  log->info("Connection worker thread started");

  return 0;
};

int ConnectionWorker::Stop() {
  char s[] = "s";
  log->debug("Sending kill pill %d", connection_pipefd[1]);
  int result = write(connection_pipefd[1], (void*)&s, 1);
  if (result == -1) {
    log->fatal("Pipe write error: %s", strerror(errno));
  }

  thread.join();

  return 0;
};

ConnectionWorker::~ConnectionWorker() {};

void* ConnectionWorker::Handler() {
  log->debug("Worker thread starting");

  status = WORKER_STARTED;

  struct epoll_event ev = {0};
  struct epoll_event* events;

  events = (epoll_event*) calloc(MAXCONNEVENTS, sizeof(epoll_event));

  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    log->error("epoll_create1 error: %s", strerror(errno));
  }

  // Added notice/pipe fd to epoll set
  ev.events = EPOLLIN;
  ev.data.fd = noticefd;

  log->debug("Worker adding notice fd (%d) to epoll", noticefd);

  int epctl = epoll_ctl(epollfd, EPOLL_CTL_ADD, noticefd, &ev);
  if (epctl == -1) {
    log->fatal("epoll_ctl noticefd error: %s", strerror(errno));
    return (void*) -1;
  }

  int n;
  int result = 0;

  while (status != WORKER_SHUTDOWN) {
    log->debug("marco");

    // Grab all the incoming connections
    connection* conn;
    while (conn = queue->pop()) {
      log->debug("Worker got a connection from queue (%d)", conn->fd);

      result = handle_connection(conn);
      if (result != 0) {
	log->error("conn handler error: %s", result);
      }
    }

    log->debug("Worker epoll");
    
    // Blocking epoll wait
    int nfds = epoll_wait(epollfd, events, MAXCONNEVENTS, 1000);
    // Check if no events
    if (nfds == 0) {
      log->debug("Worker found no epoll events");
      continue; 
    }

    log->debug("Worker found %d epoll events", nfds);

    // Handle events one at a time
    for (n = 0; n < nfds; n++) {
      if (events[n].data.fd == noticefd) {
	log->debug("Worker notice");

	// New connection event
	result = handle_notice(&events[n]);
	if (result != 0) {
	  log->error("conn handler error: %s", result);
	}
      } else {
	log->debug("Worker data");

	// Connection data event
	result = handle_data(&events[n]);
	if (result != 0) {
	  log->error("data handler error: %s", result);
	}
      }	
    }

    log->debug("polo");
  }

  log->info("Worker exiting");

  return 0;
};

int ConnectionWorker::handle_connection(connection* conn) {
  int flags = fcntl(conn->fd, F_GETFL);
  if (flags == -1) {
    log->error("client get epoll_ctl error: %s", strerror(errno));
    return -1;
  }

  flags |= O_NONBLOCK;

  int cfdctl = fcntl(conn->fd, F_SETFL, flags);
  if (cfdctl == -1) {
    log->error("client set epoll_ctl error: %s", strerror(errno));
    return -1;
  }
 

  struct epoll_event clientev = {0};
  clientev.events = EPOLLIN | EPOLLET;
  clientev.data.fd = conn->fd;
 
  log->debug("Worker adding connection fd (%d) to epoll", conn->fd);
 
  int epctl = epoll_ctl(epollfd, EPOLL_CTL_ADD, conn->fd, &clientev);
  if (epctl == -1) {
    log->error("epoll_ctl add error: %s", strerror(errno));
    return -1;
  }

  connections[conn->fd] = conn;

  return 0;
};

int ConnectionWorker::handle_notice(struct epoll_event *ev) {
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
};

int ConnectionWorker::handle_data(struct epoll_event *ev) {
  size_t bufferLen = 2000;
  char buffer[bufferLen];
  ssize_t len;
  int clientfd = ev->data.fd;
  int eventType = ev->events;
  
  connection* conn;
  try {
    conn = connections.at(clientfd);
  } catch (std::out_of_range e) {
    log->error( "bad fd (%d)", clientfd);
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

    log->debug("recieved data %d", len);

    if (conn->parser->upgrade) {
      log->warn("Protocol renegotiation");
      // protocol renegotiation, @TODO @WEBSOCKETS
    } else if (nparsed != len) {
      log->error("Protocol error");

      pool->Return(conn);
      return -1;
    }    
  }

  if (len == 0) { // FD closed
    if(conn->complete == 1) {
      log->info("What what");
    }

    log->debug("Zero length message received");

    log->debug("EPOLLIN: %c", EPOLLIN & eventType == EPOLLIN ? 't' : 'f');
    log->debug("EPOLLOUT: %c", EPOLLOUT & eventType == EPOLLOUT ? 't' : 'f');
    log->debug("EPOLLRDHUP: %c", EPOLLRDHUP & eventType == EPOLLRDHUP ? 't' : 'f');
    log->debug("EPOLLPRI: %c", EPOLLPRI & eventType == EPOLLPRI ? 't' : 'f');
    log->debug("EPOLLERR: %c", EPOLLERR & eventType == EPOLLERR ? 't' : 'f');
    log->debug("EPOLLHUP: %c", EPOLLHUP & eventType == EPOLLHUP ? 't' : 'f');
    log->debug("EPOLLET: %c", EPOLLET & eventType == EPOLLET ? 't' : 'f');
    log->debug("EPOLLONESHOT: %c", EPOLLONESHOT & eventType == EPOLLONESHOT ? 't' : 'f');    
    log->debug("Size 0");

    pool->Return(conn);
    return 0;
  }

  if (len < 0) {
    if (errno != EAGAIN) {
      log->error("Recv error: %s", strerror(errno));

      pool->Return(conn);
      return -1;
    }
  }

  if (conn->complete == 1) {
    int epctl = epoll_ctl(epollfd, EPOLL_CTL_DEL, conn->fd, NULL);
    if (epctl == -1) {
      log->error("epoll_ctl del error: %s", strerror(errno));
      return -1;
    }

    log->debug("Got a reqest (%d)", conn->fd);

    int sent = send(conn->fd, response, response_length, 0);
    if (sent < response_length) {
      log->info("Didn't send all");
    }

    if (sent < 0) {
      log->debug("Send error: %s", strerror(errno));
    }  
    
    // Reset request details
    conn->url.clear();
    conn->headers.clear();
    conn->body.clear();
    conn->complete = 0;

    pool->Return(conn);

    return 0;

    // Add request to work queue
    request* req = new request();
    req->operation = conn->url;
    req->headers = conn->headers;
    req->body = conn->body;

    // Add request to request queue
    workQueue->push(req);

    // Reset request details
    conn->url.clear();
    conn->headers.clear();
    conn->body.clear();
    conn->complete = 0;
  }

  return 0;
};

void ConnectionWorker::on_shutdown() {
  if (connection_count == 0) {
    status = WORKER_SHUTDOWN;
    return;
  }

  status = WORKER_DRAIN;
};
