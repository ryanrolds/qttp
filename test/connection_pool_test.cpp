#include "connection_pool.h"
#include "gtest/gtest.h"

#include <cstdio>

TEST(ConnectionPoolTest, Init) {
  log4cpp::Priority::PriorityLevel level = log4cpp::Priority::ERROR;
  log4cpp::Category& log = logging_init(level);

  ConnectionPool *pool = new ConnectionPool(&log);
  ASSERT_EQ(pool->GetLimit(), 100);
  ASSERT_EQ(pool->GetAvailable(), 0);
}

TEST(ConnectionPoolTest, Limit) {
  log4cpp::Priority::PriorityLevel level = log4cpp::Priority::ERROR;
  log4cpp::Category& log = logging_init(level);

  ConnectionPool *pool = new ConnectionPool(&log, 200);
  ASSERT_EQ(pool->GetLimit(), 200);
}

TEST(ConnectionPoolTest, Available) {
  log4cpp::Priority::PriorityLevel level = log4cpp::Priority::ERROR;
  log4cpp::Category& log = logging_init(level);

  ConnectionPool *pool = new ConnectionPool(&log);
  ASSERT_EQ(pool->GetLimit(), 100);
  ASSERT_EQ(pool->GetAvailable(), 0);

  int fd = open("/dev/null", O_APPEND);
  if (fd == -1) {
    log.error("Open failed: %s", strerror(errno));
    ASSERT_NE(fd, -1);
  }

  connection *conn = pool->Checkout(fd);

  ASSERT_EQ(pool->GetAvailable(), 0);

  
  int result = pool->Return(conn);
  ASSERT_EQ(result, 0);

  int available = pool->GetAvailable();
  log.warn("asfasfdasf");

  ASSERT_EQ(available, 1);
};
