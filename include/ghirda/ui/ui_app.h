#pragma once

#include <string>

namespace ghirda::ui {

class UiApp {
public:
  bool initialize(std::string* error);
  void run();
};

} // namespace ghirda::ui
