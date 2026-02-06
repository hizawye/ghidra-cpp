#include "ghirda/decompiler/rule_engine.h"

namespace ghirda::decompiler {

void RuleEngine::register_rule(const Rule& rule) { rules_.push_back(rule); }
const std::vector<Rule>& RuleEngine::rules() const { return rules_; }

} // namespace ghirda::decompiler
