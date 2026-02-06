#include <iostream>

#include "ghirda/ui/ui_app.h"

int main() {
  ghirda::ui::UiApp app;
  std::string error;
  if (!app.initialize(&error)) {
    std::cerr << "ghidra_gui init failed: " << error << std::endl;
    return 1;
  }
  std::cout << "ghidra_gui: starting" << std::endl;
  app.run();
  return 0;
}
