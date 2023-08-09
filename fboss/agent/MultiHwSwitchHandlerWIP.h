// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/types.h"

#include <folly/futures/Future.h>
#include <memory>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/HwSwitchCallback.h"

namespace facebook::fboss {

class HwSwitchHandlerWIP;
class SwitchState;
class StateDelta;
class TxPacket;
class SwitchStats;
class HwSwitchFb303Stats;
struct HwSwitchStateUpdate;
struct PlatformData;

using HwSwitchHandlerInitFn = std::function<std::unique_ptr<HwSwitchHandlerWIP>(
    const SwitchID& switchId,
    const cfg::SwitchInfo& info)>;

class MultiHwSwitchHandlerWIP {
 public:
  MultiHwSwitchHandlerWIP(
      const std::map<int64_t, cfg::SwitchInfo>& switchInfoMap,
      HwSwitchHandlerInitFn hwSwitchHandlerInitFn);

  ~MultiHwSwitchHandlerWIP();

  void start();

  void stop();

  std::shared_ptr<SwitchState> stateChanged(
      const StateDelta& delta,
      bool transaction);

  void exitFatal();

  std::unique_ptr<TxPacket> allocatePacket(uint32_t size);

  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept;

  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept;

  bool sendPacketSwitchedAsync(std::unique_ptr<TxPacket> pkt) noexcept;

  bool isValidStateUpdate(const StateDelta& delta);

  void unregisterCallbacks();

  void gracefulExit(state::WarmbootState& thriftSwitchState);

  bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip);

  folly::dynamic toFollyDynamic();

  std::optional<uint32_t> getHwLogicalPortId(PortID portID);

  const PlatformData& getPlatformData() const;

  bool transactionsSupported();

  HwSwitchFb303Stats* getSwitchStats();

  folly::F14FastMap<std::string, HwPortStats> getPortStats();

  CpuPortStats getCpuPortStats();

  std::map<std::string, HwSysPortStats> getSysPortStats();

  void updateStats(SwitchStats* switchStats);

  std::map<PortID, phy::PhyInfo> updateAllPhyInfo();

  uint64_t getDeviceWatermarkBytes();

  void clearPortStats(const std::unique_ptr<std::vector<int32_t>>& ports);

  std::vector<phy::PrbsLaneStats> getPortAsicPrbsStats(int32_t portId);

  void clearPortAsicPrbsStats(int32_t portId);

  std::vector<prbs::PrbsPolynomial> getPortPrbsPolynomials(int32_t portId);

  prbs::InterfacePrbsState getPortPrbsState(PortID portId);

  void switchRunStateChanged(SwitchRunState newState);

  // platform access apis
  void onHwInitialized(HwSwitchCallback* callback);

  void onInitialConfigApplied(HwSwitchCallback* sw);

  void platformStop();

  const AgentConfig* config();

  const AgentConfig* reloadConfig();

  std::map<PortID, FabricEndpoint> getFabricReachability();

  FabricReachabilityStats getFabricReachabilityStats();

  std::vector<PortID> getSwitchReachability(SwitchID switchId);

  std::string getDebugDump();

  void fetchL2Table(std::vector<L2EntryThrift>* l2Table);

  std::string listObjects(const std::vector<HwObjectType>& types, bool cached);

  bool needL2EntryForNeighbor();

 private:
  HwSwitchHandlerWIP* getHwSwitchHandler(SwitchID id);

  folly::Future<std::shared_ptr<SwitchState>> stateChanged(
      SwitchID switchId,
      const HwSwitchStateUpdate& update);

  std::shared_ptr<SwitchState> getStateUpdateResult(
      SwitchID switchId,
      folly::Future<std::shared_ptr<SwitchState>>&& future);

  std::map<SwitchID, std::unique_ptr<HwSwitchHandlerWIP>> hwSwitchSyncers_;
  std::atomic<bool> stopped_{true};
};

} // namespace facebook::fboss
