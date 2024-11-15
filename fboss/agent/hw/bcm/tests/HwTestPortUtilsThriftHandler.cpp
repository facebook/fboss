// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#include <bcm/port.h>
}

using facebook::fboss::cfg::PortLoopbackMode;

namespace {
facebook::fboss::cfg::PortLoopbackMode bcmToFbLoopbackMode(
    bcm_port_loopback_t inMode) {
  switch (inMode) {
    case BCM_PORT_LOOPBACK_NONE:
      return facebook::fboss::cfg::PortLoopbackMode::NONE;
    case BCM_PORT_LOOPBACK_PHY:
      return facebook::fboss::cfg::PortLoopbackMode::PHY;
    case BCM_PORT_LOOPBACK_MAC:
      return facebook::fboss::cfg::PortLoopbackMode::MAC;
    default:
      CHECK(0) << "Should never reach here";
      break;
  }
  return facebook::fboss::cfg::PortLoopbackMode::NONE;
}
} // namespace

namespace facebook {
namespace fboss {
namespace utility {

void HwTestThriftHandler::injectFecError(
    [[maybe_unused]] std::unique_ptr<std::vector<int>> hwPorts,
    [[maybe_unused]] bool injectCorrectable) {
  // not implemented in bcm
  return;
}

void HwTestThriftHandler::getPortInfo(
    ::std::vector<::facebook::fboss::utility::PortInfo>& portInfos,
    std::unique_ptr<::std::vector<::std::int32_t>> portIds) {
  for (const auto& portId : *portIds) {
    PortInfo portInfo;
    int loopbackMode;
    auto rv = bcm_port_loopback_get(
        0, static_cast<bcm_port_t>(portId), &loopbackMode);
    bcmCheckError(rv, "Failed to get loopback mode for port:", portId);
    portInfo.loopbackMode() = static_cast<int>(
        bcmToFbLoopbackMode(static_cast<bcm_port_loopback_t>(loopbackMode)));
    portInfos.push_back(portInfo);
  }
  return;
}

bool HwTestThriftHandler::verifyPortLedStatus(int /*portId*/, bool /*status*/) {
  return true;
}

} // namespace utility
} // namespace fboss
} // namespace facebook
