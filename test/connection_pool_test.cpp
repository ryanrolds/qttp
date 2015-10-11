#include "connection_pool.h"
#include "gtest/gtest.h"

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
