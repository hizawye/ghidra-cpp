#pragma once

#include <string>

#include "ghirda/core/program.h"

namespace ghirda::loader {

class Loader {
public:
  virtual ~Loader() = default;
  virtual bool load(const std::string& path, ghirda::core::Program* program, std::string* error) = 0;
};

} // namespace ghirda::loader
