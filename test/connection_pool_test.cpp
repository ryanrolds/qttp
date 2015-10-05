#include "connection_pool.h"
#include "gtest/gtest.h"

#include <vector>

namespace {
  class ConnectionPoolTest : public ::testing::Test {
  protected:
    ConnectionPoolTest() {};
    virtual ~ConnectionPoolTest() {};
    virtual void SetUp() {};
    virtual void TearDown() {};
  };

  TEST(ConnectionPoolTest, Init) {
    ConnectionPool *pool = new ConnectionPool();
    ASSERT_EQ(pool->getLimit(), 100);
    ASSERT_EQ(pool->getAvailable(), 0);
  }

  TEST(ConnectionPoolTest, Limit) {
    ConnectionPool *pool = new ConnectionPool(200);
    ASSERT_EQ(pool->getLimit(), 200);
  }

  TEST(ConnectionPoolTest, Available) {
    ConnectionPool *pool = new ConnectionPool();
    ASSERT_EQ(pool->getLimit(), 100);
    ASSERT_EQ(pool->getAvailable(), 0);

    connection *conn = pool->aquire();
    ASSERT_EQ(pool->getAvailable(), 0);

    pool->release(conn);
    ASSERT_EQ(pool->getAvailable(), 1);
  }

  TEST(ConnectionPoolTest, HitLimit) {
    ConnectionPool *pool = new ConnectionPool();
    int limit = pool->getLimit();
    std::vector<connection*> conns;

    for (int i = 0; i < limit; i++) {
      conns.push_back(pool->aquire());
    }

    ASSERT_EQ(pool->getAvailable(), 0);
    ASSERT_EQ(conns.size(), 100);

    while (conns.empty() == false) {
      pool->release(conns.back());
      conns.pop_back();
    }

    ASSERT_EQ(conns.size(), 0);
    ASSERT_EQ(pool->getAvailable(), 100);
  }
}
