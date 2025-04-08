// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/PacketSnooper.h"
#include <string>

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"

DECLARE_bool(rx_vlan_untagged_packets);

namespace facebook::fboss::utility {
PacketSnooper::PacketSnooper(
    std::optional<PortID> port,
    std::optional<utility::EthFrame> expectedFrame,
    PacketComparatorFn packetComparator)
    : port_(port),
      expectedFrame_(std::move(expectedFrame)),
      packetComparator_(std::move(packetComparator)) {
  if (FLAGS_rx_vlan_untagged_packets) {
    /* strip vlans as rx will always be untagged */
    expectedFrame_->stripVlans();
  }
}

void PacketSnooper::packetReceived(const RxPacket* pkt) noexcept {
  XLOG(DBG2) << "pkt received on port " << pkt->getSrcPort();
  if (port_.has_value() && port_.value() != pkt->getSrcPort()) {
    // packet arrived on port not of interest.
    return;
  }
  auto data = pkt->buf()->clone();
  folly::io::Cursor cursor{data.get()};
  auto frame = std::make_unique<utility::EthFrame>(cursor);

  if (packetComparator_.has_value() && expectedFrame_.has_value()) {
    if (!packetComparator_.value()(*expectedFrame_, *frame)) {
      XLOG(DBG2) << " Unexpected packet received "
                 << " expected: " << *expectedFrame_ << std::endl
                 << " got: " << *frame;
      return;
    }
  } else if (expectedFrame_.has_value() && *expectedFrame_ != *frame) {
    XLOG(DBG2) << " Unexpected packet received "
               << " expected: " << *expectedFrame_ << std::endl
               << " got: " << *frame;
    return;
  }

  std::lock_guard<std::mutex> lock(mtx_);
  receivedFrames_.push(std::move(frame));
  cv_.notify_all();
}

std::optional<utility::EthFrame> PacketSnooper::waitForPacket(
    uint32_t timeout_s) {
  std::unique_lock<std::mutex> lock(mtx_);

  while (receivedFrames_.empty()) {
    if (timeout_s > 0) {
      if (cv_.wait_for(lock, std::chrono::seconds(timeout_s)) ==
          std::cv_status::timeout) {
        return {};
      }
    } else {
      cv_.wait(lock);
    }
  }
  utility::EthFrame ret = *receivedFrames_.front();
  receivedFrames_.pop();
  return ret;
}

SwSwitchPacketSnooper::SwSwitchPacketSnooper(
    SwSwitch* sw,
    const std::string& name,
    std::optional<PortID> port,
    std::optional<utility::EthFrame> expectedFrame,
    PacketComparatorFn packetComparator)
    : PacketSnooper(
          port,
          std::move(expectedFrame),
          std::move(packetComparator)),
      sw_(sw),
      name_(name) {
  sw_->getPacketObservers()->registerPacketObserver(this, name_);
}

SwSwitchPacketSnooper::~SwSwitchPacketSnooper() {
  sw_->getPacketObservers()->unregisterPacketObserver(this, name_);
}

std::optional<std::unique_ptr<folly::IOBuf>>
SwSwitchPacketSnooper::waitForPacket(uint32_t timeout_s) {
  auto frame = PacketSnooper::waitForPacket(timeout_s);
  if (!frame) {
    return {};
  }
  auto buf = frame->toIOBuf();
  return std::move(buf);
}

} // namespace facebook::fboss::utility
