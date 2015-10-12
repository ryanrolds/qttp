#include "connection_pool.h"
#include "gtest/gtest.h"

#include <chrono>
#include <thread>
#include <vector>

void* testThread(ConnectionPool *pool) {  
  for (int i = 0; i < 10; i++) {
    connection *conn = pool->aquire();
      
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
	
    pool->release(conn);
  }
      
  return 0;
};

class ConnectionPoolTestThreaded : public ::testing::Test {
protected:
  ConnectionPool *pool;

  virtual void SetUp() {
    std::vector<std::thread*> threads;
      
    pool = new ConnectionPool(100);

    for (int i = 0; i < 200; i++) {
      std::thread *t = new std::thread(testThread, pool);
      threads.push_back(t);
    }

    while (!threads.empty()) {
      threads.back()->join();
      threads.pop_back();
    }
  };
  virtual void TearDown() {};
};

TEST_F(ConnectionPoolTestThreaded, HitLimit) {
  ASSERT_EQ(pool->getLimit(), 100);
  ASSERT_EQ(pool->getAvailable(), 100);
};
