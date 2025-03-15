// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook {
namespace fboss {
namespace utility {

void HwTestThriftHandler::printDiagCmd(std::unique_ptr<::std::string> cmd) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch_);
  saiSwitch->printDiagCmd(*cmd);
}
} // namespace utility
} // namespace fboss
} // namespace facebook
