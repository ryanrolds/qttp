#include "qttp.h"
//#include "logging.h"
#include "gtest/gtest.h"

#include <iostream>
#include <string.h>
#include <stdlib.h>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>


class QTTPTest : public ::testing::Test {
protected:
  log4cpp::Priority::PriorityLevel level = log4cpp::Priority::ERROR;
  log4cpp::Category& log = logging_init(level);

  QTTP *qttp = new QTTP(&log);

  virtual void SetUp() {
    // Bind to ephemercal port (0)
    qttp->Start(0);
  };
  virtual void TearDown() {
    qttp->Stop();

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
