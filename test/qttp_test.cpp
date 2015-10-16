#include "qttp.h"
#include "gtest/gtest.h"

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

using namespace curlpp::options;

class QTTPTest : public ::testing::Test {
protected:
  QTTP *qttp = new QTTP();

  virtual void SetUp() {
    qttp->Bind(0);
    qttp->AcceptConnections();
    qttp->StartWorkers();
  };
  virtual void TearDown() {
    qttp->StopConnections();
    qttp->StopListening();
    qttp->StopWorkers();

    delete qttp;
  };
};

TEST_F(QTTPTest, Init) {
  try {
  curlpp::Cleanup myCleanup;

  curlpp::Easy myRequest;

  myRequest.setOpt<Url>("http://google.com");
  myRequest.perform();
  } catch(curlpp::RuntimeError & e) {
    std::cout << e.what() << std::endl;
  } catch(curlpp::LogicError & e) {
    std::cout << e.what() << std::endl;
  }
};

