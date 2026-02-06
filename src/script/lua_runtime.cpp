#include "ghirda/script/lua_runtime.h"

namespace ghirda::script {

bool LuaRuntime::initialize(std::string*) { return true; }

bool LuaRuntime::run_file(const std::string&, std::string* error) {
  if (error) {
    *error = "Lua runtime not implemented";
  }
  return false;
}

} // namespace ghirda::script
