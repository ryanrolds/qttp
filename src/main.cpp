#include "qttp.h"

#include <cstddef>
#include <iostream>


int main(int argc, char *argv[]) {
  QTTP *qttp = new QTTP();
  int result = qttp->Bind();
  if (result == -1) {
    std::cout << "Error binding\n";
    return -1;
  }

  result = qttp->StartWorkers();
  if (result == -1) {
    std::cout << "Error starting workers\n";
    return -1;
  }

  qttp->Listen();

  // @TODO Signal handling

  qttp->StartTTY();

  // Cleanup
  delete qttp;

  return 0;
}
