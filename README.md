
# QTTP

## Overview

## Roadmap

### v1.0.0 
#### Complete
- Accept thread
- Priority queue
- Worker pool (select style)
- Orderly shutdown
- Worker pool (epoll style)
- Signal handling
- HTTP header and body handling
- EAGAIN handling
- C10K+ (tested up to 20k connections)
- connection worker
- connection queue
- connection object pool
- Log4cpp
- OOPing connection handler
- Fixed pool deadlock
- Created connection threads

#### Active

Adding work queue and worker threads that will handle request procesing. This
approach should allow keeplive to work, and lay ground work for persistent
connections (web sockets).

#### To do
- write unit tests for existing code, including multi-treaded tests
- write integration tests for existing HTTP handling
- .shp to halfedge data structure
- create test world/plane
- get world chunk request
- update position
- limit world chunk requests based on position
- LOD
- HTTP keepalive
- WebSockets

#### Backlog
- Access log
- RabbitMQ thread

#### Suggestions

## Setup notes

### Required packages

    # apt-get install cmake libgtest-dev libcurlpp-dev libcurl4-gnutls-dev \
        liblog4cpp5-dev

### Building

    $ cmake CMakeLists.txt
    $ make

### Testing

    $ cmake CMakeLists.txt
    $ make
    $ make test

### Up nofile limits

Open fg limit needs to be updated. Set user's "hard" and "soft" "nofile" 
in /etc/security/limits.conf. Restart or logout required.  

### Make sure to increase net.core.somaxconn (2048?)

Add entry to "/etc/sysctl.conf":

    net.core.somaxconn = 1024

It can also be set immediately:

    # sysctl -w net.core.somaxconn=1024