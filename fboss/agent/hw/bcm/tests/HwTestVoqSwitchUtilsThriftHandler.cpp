// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook {
namespace fboss {
namespace utility {
void HwTestThriftHandler::injectSwitchReachabilityChangeNotification() {
  throw FbossError("Switch reachability change notification is not supported!");
}
} // namespace utility
} // namespace fboss
} // namespace facebook
