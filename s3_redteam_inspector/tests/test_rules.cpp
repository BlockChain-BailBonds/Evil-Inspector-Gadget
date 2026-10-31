#include <cassert>
#include <cstring>
#include "risk_rules.h"
int main() {
  using namespace mermaid;
  assert(severity(Security::Wep) == Severity::High);
  assert(severity(Security::Wpa1) == Severity::Medium);
  assert(severity(Security::MixedWpa) == Severity::Medium);
  assert(severity(Security::Open) == Severity::Info);
  assert(severity(Security::Wpa2) == Severity::None);
  assert(severity(Security::Wpa3) == Severity::None);
  assert(severity(Security::Unknown) == Severity::None);
  assert(std::strcmp(indicator(Security::Wep), "LEGACY_WEP") == 0);
  assert(std::strcmp(severityName(Severity::High), "HIGH") == 0);
  return 0;
}
