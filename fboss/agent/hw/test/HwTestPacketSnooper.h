// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/packet/PktFactory.h"

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <condition_variable>

namespace facebook::fboss {

class RxPacket;

class HwTestPacketSnooper : public HwSwitchEnsemble::HwSwitchEventObserverIf {
 public:
  explicit HwTestPacketSnooper(HwSwitchEnsemble* ensemble);
  virtual ~HwTestPacketSnooper() override;
  void packetReceived(RxPacket* pkt) noexcept override;
  // Wait until timeout (seconds), If timeout = 0, wait forever.
  std::optional<utility::EthFrame> waitForPacket(uint32_t timeout_s = 0);

 private:
  void linkStateChanged(PortID /*port*/, bool /*up*/) override {}
  void l2LearningUpdateReceived(
      L2Entry /*l2Entry*/,
      L2EntryUpdateType /*l2EntryUpdateType*/) override {}

  HwSwitchEnsemble* ensemble_;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::unique_ptr<folly::IOBuf> data_;
};

} // namespace facebook::fboss
