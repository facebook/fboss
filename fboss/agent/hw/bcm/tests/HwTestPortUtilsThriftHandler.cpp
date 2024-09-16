// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook {
namespace fboss {
namespace utility {

void HwTestThriftHandler::injectFecError(
    [[maybe_unused]] std::unique_ptr<std::vector<int>> hwPorts,
    [[maybe_unused]] bool injectCorrectable) {
  // not implemented in bcm
  return;
}
} // namespace utility
} // namespace fboss
} // namespace facebook
