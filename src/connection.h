#pragma once

#include "http_parser.h"

#include <map>
#include <string>
#include <vector>

struct connection {
  int fd;
  http_parser *parser;
  std::string url;
  std::string field;
  std::string value;
  std::map<std::string, std::string> headers;
  std::vector<char> body;
  int complete = 0;
};
