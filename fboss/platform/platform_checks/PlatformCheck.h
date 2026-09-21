#pragma once

#include <set>
#include <string>

#include "fboss/platform/platform_checks/gen-cpp2/check_types_types.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_checks {

/**
 * Abstract base class for all platform checks.
 */
class PlatformCheck {
 public:
  virtual ~PlatformCheck() = default;

  virtual CheckResult run() = 0;

  virtual CheckType getType() const = 0;

  /**
   * human-readable description of what this check validates
   */
  virtual std::string getDescription() const = 0;

  virtual std::string getName() const = 0;

  virtual std::set<std::string> getSupportedPlatforms() const {
    return {}; // Empty set = all platforms supported
  }

 protected:
  /**
   * Get platform configuration. Mockable for unit tests.
   */
  virtual platform_manager::PlatformConfig getPlatformConfig() const;

  CheckResult makeError(const std::string& errorMessage) const {
    CheckResult result;
    result.checkType() = getType();
    result.checkName() = getName();
    result.status() = CheckStatus::ERROR;
    result.errorMessage() = errorMessage;
    return result;
  }

  CheckResult makeOK() const {
    CheckResult result;
    result.checkType() = getType();
    result.checkName() = getName();
    result.status() = CheckStatus::OK;
    return result;
  }

  CheckResult makeProblem(
      const std::string& errorMsg,
      RemediationType type,
      const std::string& remediation) const {
    CheckResult result;
    result.checkType() = getType();
    result.checkName() = getName();
    result.status() = CheckStatus::PROBLEM;
    result.errorMessage() = errorMsg;
    result.remediation() = type;
    result.remediationMessage() = remediation;
    return result;
  }
};

} // namespace facebook::fboss::platform::platform_checks
