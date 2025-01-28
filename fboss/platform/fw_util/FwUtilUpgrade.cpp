#include <folly/logging/xlog.h>
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::fw_util {

void FwUtilImpl::doUpgrade(const std::string& fpd) {
  XLOG(INFO) << "Running Upgrade operation for " << fpd;
  for (const auto& operation :
       *fwUtilConfig_.newFwConfigs()->at(fpd).upgrade()) {
    doUpgradeOperation(operation, fpd);
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
      performJamUpgrade(upgradeConfig.jamArgs().value(), fpd);
    }
  } else if (*upgradeConfig.commandType() == "xapp") {
    if (upgradeConfig.xappArgs().has_value()) {
      performXappUpgrade(upgradeConfig.xappArgs().value(), fpd);
    }
  } else {
    throw std::runtime_error(
        "Unsupported command type: " + *upgradeConfig.commandType());
  }
}

} // namespace facebook::fboss::platform::fw_util
