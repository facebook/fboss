// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

namespace facebook::fboss {

class HwTestLinkScanUpdateObserver
    : public HwSwitchEnsemble::HwSwitchEventObserverIf {
 public:
  explicit HwTestLinkScanUpdateObserver(HwSwitchEnsemble* ensemble);
  virtual ~HwTestLinkScanUpdateObserver() override;

  void linkStateChanged(PortID /*port*/, bool /*up*/) override {}

 private:
  // Empty private definitions since HwTestLinkScanUpdateObserver observer
  // only cares about linkStateChanged
  void packetReceived(RxPacket* /*pkt*/) noexcept override {}
  void l2LearningUpdateReceived(
      L2Entry /*l2Entry*/,
      L2EntryUpdateType /*l2EntryUpdateType*/) override {}
  void linkActiveStateChangedOrFwIsolated(
      const std::map<PortID, bool>& /*port2IsActive */,
      bool /* fwIsolated */,
      const std::optional<uint32_t>& /* numActiveFabricPortsAtFwIsolate */)
      override {}
  void linkConnectivityChanged(
      const std::map<PortID, multiswitch::FabricConnectivityDelta>&
      /*port2OldAndNewConnectivity*/) override {}
  void switchReachabilityChanged(
      const SwitchID /*switchId*/,
      const std::map<SwitchID, std::set<PortID>>& /*switchReachabilityInfo*/)
      override {}
  HwSwitchEnsemble* ensemble_;
};

} // namespace facebook::fboss
