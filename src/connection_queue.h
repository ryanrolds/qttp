#pragma once

#include "connection.h"

class ConnectionQueue {
  private:
 
 public:
  ConnectionQueue();
  ~ConnectionQueue();
  int Push(connection*);
  connection* Pop();
};
