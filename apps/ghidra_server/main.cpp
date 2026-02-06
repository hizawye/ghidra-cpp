#include <iostream>

#include "ghirda/server/server.h"

int main() {
  ghirda::server::Server server;
  std::string error;
  if (!server.start(13100, &error)) {
    std::cerr << "ghidra_server failed: " << error << std::endl;
    return 1;
  }
  std::cout << "ghidra_server: running" << std::endl;
  server.stop();
  return 0;
}
