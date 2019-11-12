// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/packet/PktFactory.h"

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <condition_variable>

namespace facebook {
namespace fboss {

class RxPacket;

class HwTestPacketSnooper : public HwSwitchEnsemble::HwSwitchEventObserverIf {
 public:
  explicit HwTestPacketSnooper(HwSwitchEnsemble* ensemble);
  virtual ~HwTestPacketSnooper() override;
  void packetReceived(RxPacket* pkt) noexcept override;
  std::optional<utility::EthFrame> waitForPacket();

 private:
  void linkStateChanged(PortID /*port*/, bool /*up*/) override {}
  HwSwitchEnsemble* ensemble_;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::unique_ptr<folly::IOBuf> data_;
};

} // namespace fboss
} // namespace facebook
