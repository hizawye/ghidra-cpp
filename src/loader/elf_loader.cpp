#include "ghirda/loader/elf_loader.h"

namespace ghirda::loader {

bool ElfLoader::load(const std::string&, ghirda::core::Program*, std::string* error) {
  if (error) {
    *error = "ELF loader not implemented";
  }
  return false;
}

} // namespace ghirda::loader
