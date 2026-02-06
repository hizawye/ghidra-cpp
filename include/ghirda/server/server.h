#pragma once

#include <cstdint>
#include <string>

namespace ghirda::server {

class Server {
public:
  bool start(uint16_t port, std::string* error);
  void stop();
};

} // namespace ghirda::server
