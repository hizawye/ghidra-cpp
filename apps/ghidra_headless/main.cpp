#include <iostream>

#include "ghirda/decompiler/decompiler.h"
#include "ghirda/loader/elf_loader.h"

int main() {
  std::cout << "ghidra_headless: not implemented" << std::endl;
  ghirda::core::Program program("sample");
  ghirda::loader::ElfLoader loader;
  std::string error;
  loader.load("", &program, &error);
  return 0;
}
