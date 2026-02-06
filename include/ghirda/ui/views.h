#pragma once

#include <string>

namespace ghirda::ui {

class ViewRegistry {
public:
  void register_view(const std::string& id);
};

} // namespace ghirda::ui
