#include <folly/logging/xlog.h>
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/helpers/PlatformUtils.h"

namespace facebook::fboss::platform::fw_util {

void FwUtilImpl::performVerify(
    const VerifyFirmwareOperationConfig& verifyConfig,
    const std::string& fpd) {
  // Perform a verify operation on the firmware device
  XLOG(INFO) << "Running verify operation for " << fpd;

  if (*verifyConfig.commandType() == "flashrom") {
    if (verifyConfig.flashromArgs().has_value()) {
      performFlashromVerify(verifyConfig.flashromArgs().value(), fpd);
    }
  } else {
    throw std::runtime_error(
        "Unsupported verify command type: " + *verifyConfig.commandType() +
        " for " + fpd);
  }
}

} // namespace facebook::fboss::platform::fw_util
