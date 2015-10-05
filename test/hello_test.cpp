#include "connection_queue.h"
#include "gtest/gtest.h"

namespace {
  class ConnectionQueueTest : public ::testing::Test {
  protected:
    ConnectionQueueTest() {};
    virtual ~ConnectionQueueTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {}; 
  };

  TEST(ConnectionQueueTest, Init) {
    ConnectionQueue *queue = new ConnectionQueue();
  }
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
