#include "ghirda/loader/macho_loader.h"

namespace ghirda::loader {

bool MachoLoader::load(const std::string&, ghirda::core::Program*, std::string* error) {
  if (error) {
    *error = "Mach-O loader not implemented";
  }
  return false;
}

} // namespace ghirda::loader
