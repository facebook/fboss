#include <folly/logging/xlog.h>
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::fw_util {

void FwUtilImpl::doPreUpgrade(const std::string& fpd) {
  XLOG(INFO) << "Running preUpgrade operation for " << fpd;
  if (auto preUpgrade = fwUtilConfig_.newFwConfigs()->at(fpd).preUpgrade()) {
    for (const auto& operation : *preUpgrade) {
      doPreUpgradeOperation(operation, fpd);
    }
  }
}

void FwUtilImpl::doPreUpgradeOperation(
    const PreFirmwareOperationConfig& operation,
    const std::string& fpd) {
  if (operation.commandType()->empty()) {
    XLOG(ERR) << "Command type is not set";
    return;
  }

  const std::string& commandType = *operation.commandType();

  if (commandType == "writeToPort") {
    if (operation.writeToPortArgs().has_value()) {
      doWriteToPortOperation(*operation.writeToPortArgs(), fpd);
    } else {
      XLOG(ERR) << "WriteToPortArgs is not set for command type: "
                << commandType;
    }
  } else if (commandType == "jtag") {
    if (operation.jtagArgs().has_value()) {
      doJtagOperation(*operation.jtagArgs(), fpd);
    } else {
      XLOG(ERR) << "jtagArgs is not set for command type: " << commandType;
    }
  } else if (commandType == "gpioset") {
    if (operation.gpiosetArgs().has_value()) {
      doGpiosetOperation(*operation.gpiosetArgs(), fpd);
    } else {
      XLOG(ERR) << "gpiosetArgs is not set for command type: " << commandType;
    }
  } else {
    XLOG(ERR) << "Unsupported command type: " << commandType;
  }
}

} // namespace facebook::fboss::platform::fw_util
