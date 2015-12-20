#pragma once

#include <map>
#include <string>
#include <vector>

struct request {
  int fd; 

  std::string operation;

  std::map<std::string, std::string> headers;
  std::vector<char> body;
};
