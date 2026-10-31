#pragma once
#include <stdint.h>

namespace mermaid {
enum class Security : uint8_t {
  Open, Wep, Wpa1, MixedWpa, Wpa2, Wpa2Enterprise, Wpa3, Unknown
};
enum class Severity : uint8_t { Info, Medium, High, None };

inline Severity severity(Security value) {
  switch (value) {
    case Security::Wep: return Severity::High;
    case Security::Wpa1:
    case Security::MixedWpa: return Severity::Medium;
    case Security::Open: return Severity::Info;
    case Security::Wpa2:
    case Security::Wpa2Enterprise:
    case Security::Wpa3:
    case Security::Unknown: return Severity::None;
  }
  return Severity::None;
}
inline const char *severityName(Severity severityValue) {
  switch (severityValue) {
    case Severity::Info: return "INFO";
    case Severity::Medium: return "MEDIUM";
    case Severity::High: return "HIGH";
    case Severity::None: return "NONE";
  }
  return "NONE";
}
inline const char *indicator(Security value) {
  switch (value) {
    case Security::Open: return "OPEN_WIFI_UNENCRYPTED";
    case Security::Wep: return "LEGACY_WEP";
    case Security::Wpa1: return "LEGACY_WPA1";
    case Security::MixedWpa: return "WPA1_FALLBACK";
    case Security::Wpa2:
    case Security::Wpa2Enterprise:
    case Security::Wpa3:
    case Security::Unknown: return "NONE";
  }
  return "NONE";
}
inline const char *explanation(Security value) {
  switch (value) {
    case Security::Open: return "No link-layer Wi-Fi encryption; not proof of an exploitable device";
    case Security::Wep: return "WEP is obsolete; verify configuration with owner";
    case Security::Wpa1: return "Legacy WPA1/TKIP protection is obsolete";
    case Security::MixedWpa: return "WPA1 compatibility may permit weaker protection";
    case Security::Wpa2:
    case Security::Wpa2Enterprise:
    case Security::Wpa3:
    case Security::Unknown: return "No finding inferred from advertised security mode";
  }
  return "No finding";
}
}
