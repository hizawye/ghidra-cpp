#include "ghirda/loader/pe_loader.h"

namespace ghirda::loader {

bool PeLoader::load(const std::string&, ghirda::core::Program*, std::string* error) {
  if (error) {
    *error = "PE loader not implemented";
  }
  return false;
}

} // namespace ghirda::loader
