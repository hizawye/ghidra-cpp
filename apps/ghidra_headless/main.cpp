#include <iostream>

#include "ghirda/decompiler/decompiler.h"
#include "ghirda/loader/elf_loader.h"
#include "ghirda/sleigh/decoder.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: ghidra_headless <elf>" << std::endl;
    return 2;
  }

  ghirda::core::Program program("sample");
  ghirda::loader::ElfLoader loader;
  std::string error;
  if (!loader.load(argv[1], &program, &error)) {
    std::cerr << "load failed: " << error << std::endl;
    return 1;
  }

  std::cout << "loaded program with " << program.memory_map().regions().size() << " region(s)" << std::endl;

  ghirda::sleigh::Decoder decoder;
  std::vector<uint8_t> bytes{0x90};
  auto decode = decoder.decode(bytes, 0x1000);
  std::cout << "decoder mnemonic: " << decode.mnemonic << " pcode: " << decode.pcode.size() << std::endl;

  ghirda::decompiler::Decompiler decompiler;
  auto result = decompiler.decompile_function(program, 0x1000);
  std::cout << "decompile success: " << result.success << std::endl;
  return 0;
}
