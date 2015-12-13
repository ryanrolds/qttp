#include "connection_pool.h"
#include "gtest/gtest.h"

#include <chrono>
#include <thread>
#include <vector>

void* testThread(ConnectionPool *pool) {  
  for (int i = 0; i < 10; i++) {
    int fd = open("/dev/null", O_APPEND);
    if (fd == -1) {
      
    }

    connection *conn = pool->Checkout(fd);
      
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
	
    pool->Return(conn);
  }
      
  return 0;
};

class ConnectionPoolTestThreaded : public ::testing::Test {
protected:
  ConnectionPool *pool;

  virtual void SetUp() {      
    log4cpp::Priority::PriorityLevel level = log4cpp::Priority::ERROR;
    log4cpp::Category& log = logging_init(level);

    pool = new ConnectionPool(&log, 100);
  };
  virtual void TearDown() {};
};

TEST_F(ConnectionPoolTestThreaded, HitLimit) {
  std::vector<std::thread*> threads;
  for (int i = 0; i < 200; i++) {
    std::thread *t = new std::thread(testThread, pool);
    threads.push_back(t);
  }
 
  ASSERT_EQ(pool->GetLimit(), 100);
  ASSERT_EQ(pool->GetAvailable(), 100);

  while (!threads.empty()) {
    threads.back()->join();
    threads.pop_back();
  }  
};
