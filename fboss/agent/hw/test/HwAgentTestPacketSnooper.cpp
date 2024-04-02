// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwAgentTestPacketSnooper.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/packet/PktUtil.h"

#include <memory>

namespace facebook::fboss {

// Very similar to HwTestPacketSnooper, except that its decoupled from
// HwEnsemble and can be used by Agent tests which rely on more generic
// EventObserver
HwAgentTestPacketSnooper::HwAgentTestPacketSnooper(
    PacketObservers* observers,
    std::optional<PortID> port)
    : observers_(observers), snooper_(port) {
  observers_->registerPacketObserver(this, "TestPktSnooper");
}

HwAgentTestPacketSnooper::~HwAgentTestPacketSnooper() {
  observers_->unregisterPacketObserver(this, "TestPktSnooper");
}

void HwAgentTestPacketSnooper::packetReceived(const RxPacket* pkt) noexcept {
  snooper_.packetReceived(pkt);
}

std::optional<std::unique_ptr<folly::IOBuf>>
HwAgentTestPacketSnooper::waitForPacket(uint32_t timeout_s) {
  auto frame = snooper_.waitForPacket(timeout_s);
  if (!frame) {
    return {};
  }
  auto buf = folly::IOBuf::create(frame->length());
  buf->append(frame->length());
  folly::io::RWPrivateCursor cursor(buf.get());
  frame->serialize(cursor);
  return std::move(buf);
}

} // namespace facebook::fboss
