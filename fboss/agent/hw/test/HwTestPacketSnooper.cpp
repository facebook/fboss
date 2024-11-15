// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/packet/PktUtil.h"

namespace facebook::fboss {

HwTestPacketSnooper::HwTestPacketSnooper(
    HwSwitchEnsemble* ensemble,
    std::optional<PortID> port,
    std::optional<utility::EthFrame> expectedFrame)
    : ensemble_(ensemble), snooper_(port, expectedFrame) {
  ensemble_->addHwEventObserver(this);
}

HwTestPacketSnooper::~HwTestPacketSnooper() {
  ensemble_->removeHwEventObserver(this);
}

void HwTestPacketSnooper::packetReceived(RxPacket* pkt) noexcept {
  snooper_.packetReceived(pkt);
}

std::optional<utility::EthFrame> HwTestPacketSnooper::waitForPacket(
    uint32_t timeout_s) {
  return snooper_.waitForPacket(timeout_s);
}

} // namespace facebook::fboss
