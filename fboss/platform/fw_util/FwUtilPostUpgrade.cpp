#include <folly/logging/xlog.h>
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::fw_util {

void FwUtilImpl::doPostUpgrade(const std::string& fpd) {
  XLOG(INFO) << "Running postUpgrade operation for " << fpd;
  if (auto postUpgrade = fwUtilConfig_.newFwConfigs()->at(fpd).postUpgrade()) {
    for (const auto& operation : *postUpgrade) {
      doPostUpgradeOperation(operation, fpd);
    }
  }
}

void FwUtilImpl::doPostUpgradeOperation(
    const PostFirmwareOperationConfig& postUpgradeConfig,
    const std::string& fpd) {
  if (postUpgradeConfig.commandType() == "writeToPort" &&
      postUpgradeConfig.writeToPortArgs().has_value()) {
    doWriteToPortOperation(*postUpgradeConfig.writeToPortArgs(), fpd);
  } else if (
      postUpgradeConfig.commandType() == "gpioget" &&
      postUpgradeConfig.gpiogetArgs().has_value()) {
    doGpiogetOperation(*postUpgradeConfig.gpiogetArgs(), fpd);
  } else if (
      postUpgradeConfig.commandType() == "jtag" &&
      postUpgradeConfig.jtagArgs().has_value()) {
    doJtagOperation(*postUpgradeConfig.jtagArgs(), fpd);
  }
}

} // namespace facebook::fboss::platform::fw_util
