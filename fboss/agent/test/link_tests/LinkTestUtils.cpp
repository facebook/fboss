// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/link_tests/LinkTestUtils.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

namespace facebook::fboss::utility {

const TransceiverSpec* getTransceiverSpec(const SwSwitch* sw, PortID portId) {
  auto platformPort = sw->getPlatformMapping()->getPlatformPort(portId);
  const auto& chips = sw->getPlatformMapping()->getChips();
  if (auto tcvrID = utility::getTransceiverId(platformPort, chips)) {
    auto transceiver = sw->getState()->getTransceivers()->getNodeIf(*tcvrID);
    return transceiver.get();
  }
  return nullptr;
}

} // namespace facebook::fboss::utility
