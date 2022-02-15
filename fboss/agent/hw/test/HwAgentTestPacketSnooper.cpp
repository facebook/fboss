// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwAgentTestPacketSnooper.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/packet/PktUtil.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

// Very similar to HwTestPacketSnooper, except that its decoupled from
// HwEnsemble and can be used by Agent tests which rely on more generic
// EventObserver
HwAgentTestPacketSnooper::HwAgentTestPacketSnooper(
    PacketObservers* observers,
    std::optional<PortID> port)
    : observers_(observers), ingressPort_(port) {
  observers_->registerPacketObserver(this, "TestPktSnooper");
}

HwAgentTestPacketSnooper::~HwAgentTestPacketSnooper() {
  observers_->unregisterPacketObserver(this, "TestPktSnooper");
}

void HwAgentTestPacketSnooper::packetReceived(const RxPacket* pkt) noexcept {
  XLOG(DBG5) << "pkt received on port " << pkt->getSrcPort();
  if (ingressPort_.has_value() && ingressPort_.value() != pkt->getSrcPort()) {
    // packet arrived on port not of interest.
    return;
  }
  std::lock_guard<std::mutex> lock(mtx_);
  data_ = pkt->buf()->clone();
  cv_.notify_all();
}

std::optional<std::unique_ptr<folly::IOBuf>>
HwAgentTestPacketSnooper::waitForPacket(uint32_t timeout_s) {
  std::unique_lock<std::mutex> lock(mtx_);
  while (!data_) {
    if (cv_.wait_for(lock, std::chrono::seconds(timeout_s), [&]() {
          return data_ ? true : false;
        })) {
      break;
    } else {
      return {};
    }
  };
  // create another copy for  processing by invoker of this API, as we want to
  // continue snooping multiple  packets e.g PTP we want to reset data_  so
  // waitForPacket is triggerred for any new packet only else it will continue
  // to trigger for the same pkt over and over again
  auto buf =
      IOBuf::copyBuffer(data_->data(), data_->length(), data_->headroom(), 0);
  data_.reset();
  return std::move(buf);
}

} // namespace facebook::fboss
