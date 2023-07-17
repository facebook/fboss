// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/HwSwitchCallback.h"

namespace facebook::fboss {

class TxPacket;
class StateDelta;
class SwitchStats;
class HwSwitchFb303Stats;

struct PlatformData {
  std::string volatileStateDir;
  std::string persistentStateDir;
  std::string crashSwitchStateFile;
  std::string crashThriftSwitchStateFile;
  std::string warmBootDir;
  std::string crashBadStateUpdateDir;
  std::string crashBadStateUpdateOldStateFile;
  std::string crashBadStateUpdateNewStateFile;
  std::string runningConfigDumpFile;
  bool supportsAddRemovePort;
};

struct HwSwitchHandler {
  virtual ~HwSwitchHandler() = default;

  virtual HwInitResult initHw(
      HwSwitchCallback* callback,
      bool failHwCallsOnWarmboot) = 0;

  virtual void exitFatal() const = 0;

  virtual std::unique_ptr<TxPacket> allocatePacket(uint32_t size) const = 0;

  virtual bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      std::optional<uint8_t> queue = std::nullopt) noexcept = 0;

  virtual bool sendPacketSwitchedSync(
      std::unique_ptr<TxPacket> pkt) noexcept = 0;

  virtual bool sendPacketSwitchedAsync(
      std::unique_ptr<TxPacket> pkt) noexcept = 0;

  virtual bool isValidStateUpdate(const StateDelta& delta) const = 0;

  virtual void unregisterCallbacks() = 0;

  virtual void gracefulExit(
      folly::dynamic& follySwitchState,
      state::WarmbootState& thriftSwitchState) = 0;

  virtual bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) = 0;

  virtual folly::dynamic toFollyDynamic() const = 0;

  virtual std::optional<uint32_t> getHwLogicalPortId(PortID portID) const = 0;

  const PlatformData& getPlatformData() const {
    return platformData_;
  }

  virtual bool transactionsSupported() const = 0;

  virtual HwSwitchFb303Stats* getSwitchStats() const = 0;

  virtual folly::F14FastMap<std::string, HwPortStats> getPortStats() const = 0;

  virtual CpuPortStats getCpuPortStats() const = 0;

  virtual std::map<std::string, HwSysPortStats> getSysPortStats() const = 0;

  virtual void updateStats(SwitchStats* switchStats) = 0;

  virtual std::map<PortID, phy::PhyInfo> updateAllPhyInfo() = 0;

  virtual uint64_t getDeviceWatermarkBytes() const = 0;

  virtual void clearPortStats(
      const std::unique_ptr<std::vector<int32_t>>& ports) = 0;

  virtual std::vector<phy::PrbsLaneStats> getPortAsicPrbsStats(
      int32_t portId) = 0;

  virtual void clearPortAsicPrbsStats(int32_t portId) = 0;

  virtual std::vector<prbs::PrbsPolynomial> getPortPrbsPolynomials(
      int32_t portId) = 0;

  virtual prbs::InterfacePrbsState getPortPrbsState(PortID portId) = 0;

  virtual std::vector<phy::PrbsLaneStats> getPortGearboxPrbsStats(
      int32_t portId,
      phy::Side side) = 0;

  virtual void clearPortGearboxPrbsStats(int32_t portId, phy::Side side) = 0;

  virtual void switchRunStateChanged(SwitchRunState newState) = 0;

  virtual std::shared_ptr<SwitchState> stateChanged(
      const StateDelta& delta,
      bool transaction) = 0;

  virtual fsdb::OperDelta stateChanged(
      const fsdb::OperDelta& delta,
      bool transaction) = 0;

  // platform access apis
  virtual void onHwInitialized(HwSwitchCallback* callback) = 0;

  virtual void onInitialConfigApplied(HwSwitchCallback* sw) = 0;

  virtual void platformStop() = 0;

  virtual const AgentConfig* config() = 0;

  virtual const AgentConfig* reloadConfig() = 0;

  virtual std::map<PortID, FabricEndpoint> getFabricReachability() const = 0;

  virtual FabricReachabilityStats getFabricReachabilityStats() const = 0;

  virtual std::vector<PortID> getSwitchReachability(
      SwitchID switchId) const = 0;

  virtual std::string getDebugDump() const = 0;

  virtual void fetchL2Table(std::vector<L2EntryThrift>* l2Table) const = 0;

  virtual std::string listObjects(
      const std::vector<HwObjectType>& types,
      bool cached) const = 0;

  virtual bool needL2EntryForNeighbor() const = 0;

 protected:
  virtual void initPlatformData() = 0;
  PlatformData platformData_;
};

} // namespace facebook::fboss
