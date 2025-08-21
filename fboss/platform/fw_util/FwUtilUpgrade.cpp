#include <folly/logging/xlog.h>
#include "fboss/platform/fw_util/FwUtilImpl.h"

namespace facebook::fboss::platform::fw_util {

void FwUtilImpl::doUpgrade(const std::string& fpd) {
  if (dryRun_) {
    XLOG(INFO) << "Dry run mode enabled, skipping upgrade for " << fpd;
    return;
  }
  XLOG(INFO) << "Running Upgrade operation for " << fpd;
  if (fwUtilConfig_.fwConfigs()->at(fpd).upgrade().has_value()) {
    for (const auto& operation :
         fwUtilConfig_.fwConfigs()->at(fpd).upgrade().value()) {
      doUpgradeOperation(operation, fpd);
    }
  } else {
    throw std::runtime_error("No upgrade operation found for " + fpd);
  }
}

void FwUtilImpl::doUpgradeOperation(
    const UpgradeConfig& upgradeConfig,
    const std::string& fpd) {
  if (*upgradeConfig.commandType() == "flashrom") {
    if (upgradeConfig.flashromArgs().has_value()) {
      performFlashromUpgrade(upgradeConfig.flashromArgs().value(), fpd);
    }
  } else if (*upgradeConfig.commandType() == "jam") {
    if (upgradeConfig.jamArgs().has_value()) {
      performJamUpgrade(upgradeConfig.jamArgs().value());
    }
  } else if (*upgradeConfig.commandType() == "xapp") {
    if (upgradeConfig.xappArgs().has_value()) {
      performXappUpgrade(upgradeConfig.xappArgs().value());
    }
  } else {
    throw std::runtime_error(
        "Unsupported command type: " + *upgradeConfig.commandType());
  }
}

} // namespace facebook::fboss::platform::fw_util
