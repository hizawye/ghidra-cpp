#pragma once

#include "ghirda/loader/loader.h"

namespace ghirda::loader {

class MachoLoader : public Loader {
public:
  bool load(const std::string& path, ghirda::core::Program* program, std::string* error) override;
};

} // namespace ghirda::loader
