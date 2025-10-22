#pragma once

#include <folly/MacAddress.h>

#include "fboss/platform/platform_checks/PlatformCheck.h"

namespace facebook::fboss::platform::platform_checks {

/**
 * Validates that the eth0 MAC address matches the EEPROM MAC address.
 */
class MacAddressCheck : public PlatformCheck {
 public:
  CheckResult run() override;

  CheckType getType() const override {
    return CheckType::MAC_ADDRESS_CHECK;
  }

  std::string getName() const override {
    return "Mac Address Correctness";
  }

  std::string getDescription() const override {
    return "Validates that eth0 MAC address matches EEPROM MAC address";
  }

 protected:
  virtual folly::MacAddress getMacAddress(const std::string& interface);
  virtual folly::MacAddress getEepromMacAddress();
};

} // namespace facebook::fboss::platform::platform_checks
