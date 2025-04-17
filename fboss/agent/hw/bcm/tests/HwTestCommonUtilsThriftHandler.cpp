// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook {
namespace fboss {
namespace utility {
void HwTestThriftHandler::printDiagCmd(std::unique_ptr<::std::string> cmd) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  bcmSwitch->printDiagCmd(*cmd);
}
} // namespace utility
} // namespace fboss
} // namespace facebook
