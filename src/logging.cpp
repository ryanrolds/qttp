#include "logging.h"

#include "log4cpp/Appender.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/Layout.hh"
#include "log4cpp/BasicLayout.hh"

log4cpp::Category& logging_init(log4cpp::Priority::PriorityLevel level) {
  log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
  if (appender1 == NULL) {
    throw log4cpp::ConfigureFailure("Unable to log to cout");
  }

  appender1->setLayout(new log4cpp::BasicLayout());

  std::string logFile = "logs/program.log";
  log4cpp::Appender *appender2 = new log4cpp::FileAppender("default", logFile.c_str());
  // Reopen to detect error
  if (!appender2->reopen()) {
    throw log4cpp::ConfigureFailure("Unable to open log file");
  }

  appender2->setLayout(new log4cpp::BasicLayout());

  log4cpp::Category& root = log4cpp::Category::getRoot();
  root.addAppender(appender1);
  root.addAppender(appender2);

  // Set log level
  root.setPriority(level);

  root.debug("### Logging initialized ###");

  return root;
};
