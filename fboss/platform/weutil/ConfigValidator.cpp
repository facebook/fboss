#include "fboss/platform/weutil/ConfigValidator.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::platform::weutil {

bool ConfigValidator::isValid(
    const weutil_config::WeutilConfig& config,
    const std::string& platformName) {
  std::unordered_set<std::string> uniqueNames{};
  for (const auto& [eepromName, eepromConfig] : *config.fruEepromList()) {
    XLOG(INFO) << fmt::format("Validating FruEepromConfig for {}", eepromName);
    if (eepromName.empty()) {
      XLOG(ERR) << "FruEepromName in WeutilConfig must be a non-empty string";
      return false;
    }
    // Check if EEPROM name is a unique string in the list
    auto [it, inserted] = uniqueNames.insert(eepromName);
    if (!inserted) {
      XLOG(ERR) << fmt::format("Duplicate FruEepromName: {}", eepromName);
      return false;
    }
    // Check if FruEepromPath is a non-empty string
    if (eepromConfig.path()->empty()) {
      if (platformName == "darwin" &&
          eepromName == *config.chassisEepromName()) {
        XLOG(WARN)
            << "Darwin doesn't have chassis eeprom, ignoring it as an exception";
      } else {
        XLOG(ERR) << fmt::format(
            "FruEepromPath must be a non-empty string for {}", eepromName);
        return false;
      }
    } else if (!eepromConfig.path()->starts_with("/run/devmap/eeproms")) {
      XLOG(ERR) << fmt::format(
          "FruEepromPath must start with /run/devmap/eeproms for {}, got {}",
          eepromName,
          *eepromConfig.path());
      return false;
    }
  }

  return true;
}

} // namespace facebook::fboss::platform::weutil
