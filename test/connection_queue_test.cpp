#include "connection_queue.h"
#include "connection_pool.h"
#include "gtest/gtest.h"

namespace {
  class ConnectionQueueTest : public ::testing::Test {
  protected:
    ConnectionQueueTest() {};
    virtual ~ConnectionQueueTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};
  };

  TEST(ConnectionQueueTest, Startup) {
    ConnectionQueue *queue = new ConnectionQueue();
    ASSERT_EQ(queue->running(), true);
  }

  TEST(ConnectionQueueTest, Shutdown) {
    ConnectionQueue *queue = new ConnectionQueue();
    queue->shutdown();
    ASSERT_EQ(queue->running(), false);
  }

  TEST(ConnectionQueueTest, Push) {
    ConnectionQueue *queue = new ConnectionQueue();
    queue->push(new connection());
    ASSERT_EQ(queue->size(), 1);
  }

  TEST(ConnectionQueueTest, Pop) {
    ConnectionQueue *queue = new ConnectionQueue();
    queue->push(new connection());
    connection *conn = queue->pop();
    ASSERT_EQ(queue->size(), 0);
  }
}
