#include "ghirda/script/script_api.h"

namespace ghirda::script {

ScriptApiVersion ScriptApi::version() const { return ScriptApiVersion{}; }

void ScriptApi::register_binding(const std::string&) {}

} // namespace ghirda::script
