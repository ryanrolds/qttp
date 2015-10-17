#include "logging.h"

log4cpp::Category& logging_init(log4cpp::Priority::PriorityLevel level) {
  log4cpp::Appender *appender1 = new log4cpp::OstreamAppender("console", &std::cout);
  appender1->setLayout(new log4cpp::BasicLayout());

  //log4cpp::Appender *appender2 = new log4cpp::FileAppender("default", "program.log");
  //appender2->setLayout(new log4cpp::BasicLayout());

  log4cpp::Category& root = log4cpp::Category::getRoot();
  root.addAppender(appender1);

  // Set log level
  root.setPriority(level);

  root.debug("### Logging initialized ###");

  return root;
};
