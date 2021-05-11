// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/packet/PktUtil.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

HwTestPacketSnooper::HwTestPacketSnooper(
    HwSwitchEnsemble* ensemble,
    std::optional<PortID> port)
    : ensemble_(ensemble), port_(port) {
  ensemble_->addHwEventObserver(this);
}

HwTestPacketSnooper::~HwTestPacketSnooper() {
  ensemble_->removeHwEventObserver(this);
}

void HwTestPacketSnooper::packetReceived(RxPacket* pkt) noexcept {
  XLOG(DBG2) << "pkt received on port " << pkt->getSrcPort();
  if (port_.has_value() && port_.value() != pkt->getSrcPort()) {
    // packet arrived on port not of interest.
    return;
  }
  std::lock_guard<std::mutex> lock(mtx_);
  data_ = pkt->buf()->clone();
  cv_.notify_all();
}

std::optional<utility::EthFrame> HwTestPacketSnooper::waitForPacket(
    uint32_t timeout_s) {
  std::unique_lock<std::mutex> lock(mtx_);
  while (!data_) {
    if (timeout_s > 0) {
      if (cv_.wait_for(lock, std::chrono::seconds(timeout_s)) ==
          std::cv_status::timeout) {
        return {};
      }
    } else {
      cv_.wait(lock);
    }
  }
  folly::io::Cursor cursor{data_.get()};
  return utility::EthFrame{cursor};
}

} // namespace facebook::fboss
