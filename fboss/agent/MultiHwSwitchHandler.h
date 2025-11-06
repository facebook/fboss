// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/types.h"

#include <folly/futures/Future.h>
#include <memory>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/HwSwitchCallback.h"
#include "fboss/agent/HwSwitchConnectionStatusTable.h"
#include "fboss/agent/HwSwitchHandler.h"
#include "fboss/agent/if/gen-cpp2/MultiSwitchCtrl.h"

namespace facebook::fboss {

class HwSwitchHandler;
class SwitchState;
class StateDelta;
class TxPacket;
class SwitchStats;
class HwSwitchFb303Stats;
struct HwSwitchStateUpdate;
class SwSwitch;
class AgentStats;

using HwSwitchHandlerInitFn = std::function<std::unique_ptr<HwSwitchHandler>(
    const SwitchID& switchId,
    const cfg::SwitchInfo& info,
    SwSwitch* sw)>;

class MultiHwSwitchHandler {
 public:
  MultiHwSwitchHandler(
      const std::map<int64_t, cfg::SwitchInfo>& switchInfoMap,
      HwSwitchHandlerInitFn hwSwitchHandlerInitFn,
      SwSwitch* sw,
      std::optional<cfg::SdkVersion> sdkVersion);

  ~MultiHwSwitchHandler();

  void start();

  void stop();

  bool isRunning() const {
    return !stopped_.load();
  }

  multiswitch::StateOperDelta getNextStateOperDelta(
      int64_t switchId,
      std::unique_ptr<multiswitch::StateOperDelta> prevOperResult,
      int64_t lastUpdateSeqNum);

  void notifyHwSwitchGracefulExit(int64_t switchId);

  void notifyHwSwitchDisconnected(int64_t switchId, bool gracefulExit);

  std::shared_ptr<SwitchState> stateChanged(
      const StateDelta& delta,
      bool transaction,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE);

  std::shared_ptr<SwitchState> stateChanged(
      const std::vector<StateDelta>& deltas,
      bool transaction,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE);

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size);

  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept;

  bool sendPacketOutOfPortSyncForPktType(
      std::unique_ptr<TxPacket> pkt,
      const PortID& portID,
      TxPacketType packetType) noexcept;

  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept;

  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept;

  bool transactionsSupported() const;

  std::map<PortID, FabricEndpoint> getFabricConnectivity();

  FabricReachabilityStats getFabricReachabilityStats();

  std::vector<PortID> getSwitchReachability(SwitchID switchId);

  bool needL2EntryForNeighbor(const cfg::SwitchConfig* config) const;

  // For test purpose
  std::map<SwitchID, HwSwitchHandler*> getHwSwitchHandlers() const;

  /*
   * blocks till atleast one HwSwitch is connected.
   * returns false if wait is cancelled
   */
  bool waitUntilHwSwitchConnected();

  std::map<int32_t, SwitchRunState> getHwSwitchRunStates();

  void connected(SwitchID switchId) {
    connectionStatusTable_.connected(switchId);
  }

  void disconnected(SwitchID switchId) {
    connectionStatusTable_.disconnected(switchId);
  }

  bool isHwSwitchConnected(const SwitchID& switchId);
  void fillHwAgentConnectionStatus(AgentStats& agentStats);

  state::SwitchState reconstructSwitchState(SwitchID id);

 private:
  bool transactionsSupported(std::optional<cfg::SdkVersion> sdkVersion) const;

  HwSwitchHandler* getHwSwitchHandler(SwitchID id);

  folly::Future<HwSwitchStateUpdateResult> stateChanged(
      SwitchID switchId,
      const HwSwitchStateUpdate& update,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE);

  std::map<SwitchID, HwSwitchStateUpdateResult> stateChanged(
      const std::map<SwitchID, const std::vector<StateDelta>&>& deltas,
      bool transaction,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE);

  std::map<SwitchID, HwSwitchStateUpdateResult> getStateUpdateResult(
      const std::vector<SwitchID>& switchIds,
      std::vector<folly::Future<HwSwitchStateUpdateResult>>& futures) const;

  std::shared_ptr<SwitchState> rollbackStateChange(
      const std::map<SwitchID, HwSwitchStateUpdateResult>& updateResults,
      const std::vector<StateDelta>& deltas,
      bool transaction);

  SwSwitch* sw_;
  std::map<SwitchID, std::unique_ptr<HwSwitchHandler>> hwSwitchSyncers_;
  std::atomic<bool> stopped_{true};
  HwSwitchConnectionStatusTable connectionStatusTable_;
  const bool transactionsSupported_;
};

} // namespace facebook::fboss
