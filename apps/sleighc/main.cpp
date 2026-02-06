#include <iostream>

#include "ghirda/sleigh/sleigh_compiler.h"

int main() {
  ghirda::sleigh::SleighCompiler compiler;
  ghirda::sleigh::SleighSpec spec{"default", ""};
  std::string error;
  if (!compiler.compile(spec, &error)) {
    std::cerr << "sleighc failed: " << error << std::endl;
    return 1;
  }
  std::cout << "sleighc: ok" << std::endl;
  return 0;
}
