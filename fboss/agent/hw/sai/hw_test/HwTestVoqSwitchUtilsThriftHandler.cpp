// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook {
namespace fboss {
namespace utility {
void HwTestThriftHandler::injectSwitchReachabilityChangeNotification() {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch_);
  saiSwitch->injectSwitchReachabilityChangeNotification();
}
} // namespace utility
} // namespace fboss
} // namespace facebook
