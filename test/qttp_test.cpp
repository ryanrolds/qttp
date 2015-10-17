#include "qttp.h"
#include "gtest/gtest.h"

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>


using namespace curlpp::options;

class QTTPTest : public ::testing::Test {
protected:
  QTTP *qttp = new QTTP();

  virtual void SetUp() {
    qttp->Bind(0);
    qttp->AcceptConnections();
    qttp->StartWorkers();
    qttp->Listen();
  };
  virtual void TearDown() {
    qttp->StopConnections();
    qttp->StopListening();
    qttp->StopWorkers();

    delete qttp;
  };
};

TEST_F(QTTPTest, Init) {
  std::string port = std::to_string(qttp->GetPort());

  curlpp::Cleanup myCleanup;
  curlpp::Easy request;

  // GET /ping
  curlpp::options::Url url("http://127.0.0.1:" + port + "/ping");
  request.setOpt(url);

  // Store body in body
  std::ostringstream body;
  curlpp::options::WriteStream ws(&body);
  request.setOpt(ws);

  request.perform();

  // Check status
  int status = curlpp::infos::ResponseCode::get(request);
  ASSERT_STREQ("200", std::to_string(status).c_str());

  // Check body
  ASSERT_STREQ("blah", body.str().c_str());
};
