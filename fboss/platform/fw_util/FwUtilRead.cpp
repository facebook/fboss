#include <folly/logging/xlog.h>
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::fw_util {

void FwUtilImpl::performRead(
    const ReadFirmwareOperationConfig& readConfig,
    const std::string& fpd) {
  // Perform a read operation on the firmware device
  XLOG(INFO) << "Running read operation for " << fpd;
  if (*readConfig.commandType() == "flashrom") {
    if (readConfig.flashromArgs().has_value()) {
      performFlashromRead(readConfig.flashromArgs().value(), fpd);
    }
  } else {
    throw std::runtime_error(
        "Unsupported read command type: " + *readConfig.commandType() +
        " for " + fpd);
  }
}

} // namespace facebook::fboss::platform::fw_util
