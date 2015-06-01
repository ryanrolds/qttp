#include "qttp.h"

const int NUM_WORKERS = 10;

int main(int argc, char *argv[]) {
  int result = qttp(NUM_WORKERS);
  return result;
}
