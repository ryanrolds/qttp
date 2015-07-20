
# QTTP


# Roadmap

## v1.0.0 
### Complete
- Accept thread
- Priority queue
- Worker pool (select style)
- Orderly shutdown
- Worker pool (epoll style)
- Signal handling
- HTTP header and body handling
- EAGAIN handling
- C10K+ (tested up to 20k connections)

### In progress
- connection worker
- connection queue
- connection object pool

### To do
- .shp to halfedge data structure

### Backlog
- Access log
- log4cplus
- RabbitMQ thread

### Suggestions

## Setup notes
- Up nofile limits 
- Make sure to increase net.core.somaxconn (2048?)