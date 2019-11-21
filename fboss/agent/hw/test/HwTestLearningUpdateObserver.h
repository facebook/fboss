// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/packet/PktFactory.h"

#include <folly/Optional.h>
#include <condition_variable>

namespace facebook {
namespace fboss {

class RxPacket;

class HwTestLearningUpdateObserver
    : public HwSwitchEnsemble::HwSwitchEventObserverIf {
 public:
  explicit HwTestLearningUpdateObserver(HwSwitchEnsemble* ensemble);
  virtual ~HwTestLearningUpdateObserver() override;

  void l2LearningUpdateReceived(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType) override;
  std::pair<L2Entry, L2EntryUpdateType>* waitForLearningUpdate();

 private:
  void packetReceived(RxPacket* /*pkt*/) noexcept override {}
  void linkStateChanged(PortID /*port*/, bool /*up*/) override {}

  HwSwitchEnsemble* ensemble_;
  std::mutex mtx_;
  std::condition_variable cv_;
  std::unique_ptr<std::pair<L2Entry, L2EntryUpdateType>> data_;
};

} // namespace fboss
} // namespace facebook
