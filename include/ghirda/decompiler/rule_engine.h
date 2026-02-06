#pragma once

#include <string>
#include <vector>

namespace ghirda::decompiler {

struct Rule {
  std::string name;
};

class RuleEngine {
public:
  void register_rule(const Rule& rule);
  const std::vector<Rule>& rules() const;

private:
  std::vector<Rule> rules_{};
};

} // namespace ghirda::decompiler
