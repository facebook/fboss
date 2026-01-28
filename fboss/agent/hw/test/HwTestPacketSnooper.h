// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/packet/EthFrame.h"

#include <folly/Optional.h>
#include <folly/io/IOBuf.h>
#include <condition_variable>
#include <optional>

#include "fboss/agent/test/utils/PacketSnooper.h"

namespace facebook::fboss {

class RxPacket;

class HwTestPacketSnooper : public HwSwitchEnsemble::HwSwitchEventObserverIf {
 public:
  explicit HwTestPacketSnooper(
      HwSwitchEnsemble* ensemble,
      std::optional<PortID> port = std::nullopt,
      std::optional<utility::EthFrame> expectedFrame = std::nullopt);
  virtual ~HwTestPacketSnooper() override;
  void packetReceived(RxPacket* pkt) noexcept override;
  // Wait until timeout (seconds), If timeout = 0, wait forever.
  std::optional<utility::EthFrame> waitForPacket(uint32_t timeout_s = 0);

 private:
  void linkStateChanged(PortID /*port*/, bool /*up*/) override {}
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
  utility::PacketSnooper snooper_;
};

} // namespace facebook::fboss
