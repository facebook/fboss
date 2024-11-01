// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/packet/PktFactory.h"

#include <folly/Optional.h>
#include <condition_variable>
#include <optional>
#include <vector>

namespace facebook::fboss {

class RxPacket;

class HwTestLearningUpdateObserver
    : public HwSwitchEnsemble::HwSwitchEventObserverIf {
 public:
  void startObserving(HwSwitchEnsemble* ensemble);
  void stopObserving() override;

  void reset();

  void l2LearningUpdateReceived(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType) override;
  std::vector<std::pair<L2Entry, L2EntryUpdateType>> waitForLearningUpdates(
      int numUpdates = 1,
      std::optional<int> secondsToWait = std::nullopt);
  void waitForStateUpdate();

 private:
  void packetReceived(RxPacket* /*pkt*/) noexcept override {}
  void linkStateChanged(PortID /*port*/, bool /*up*/) override {}
  void linkActiveStateChanged(
      const std::map<PortID, bool>& /*port2IsActive */) override {}
  void switchReachabilityChanged(
      const SwitchID /*switchId*/,
      const std::map<SwitchID, std::set<PortID>>& /*switchReachabilityInfo*/)
      override {}
  void linkConnectivityChanged(
      const std::map<PortID, multiswitch::FabricConnectivityDelta>&
      /*port2OldAndNewConnectivity*/) override {}

  void applyStateUpdateHelper(
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType);

  HwSwitchEnsemble* ensemble_{nullptr};
  std::mutex mtx_;
  std::condition_variable cv_;
  std::vector<std::pair<L2Entry, L2EntryUpdateType>> data_;

  std::unique_ptr<std::thread> applyStateUpdateThread_;
  FbossEventBase applyStateUpdateEventBase_{"ApplyStateUpdateEventBase"};
};

class HwTestLearningUpdateAutoObserver : public HwTestLearningUpdateObserver {
 public:
  explicit HwTestLearningUpdateAutoObserver(HwSwitchEnsemble* ensemble);
  ~HwTestLearningUpdateAutoObserver();

 private:
  HwSwitchEnsemble* ensemble_;
};

} // namespace facebook::fboss
