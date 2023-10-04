// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/HwSwitchCallback.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/types.h"

#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>
#include "fboss/agent/if/gen-cpp2/MultiSwitchCtrl.h"

namespace facebook::fboss {

class StateDelta;
class TxPacket;
class SwitchStats;
class HwSwitchFb303Stats;

struct HwSwitchStateUpdate {
  HwSwitchStateUpdate(const StateDelta& delta, bool transaction);
  std::shared_ptr<SwitchState> oldState;
  std::shared_ptr<SwitchState> newState;
  fsdb::OperDelta inDelta;
  bool isTransaction;
};

enum HwSwitchStateUpdateStatus {
  HWSWITCH_STATE_UPDATE_SUCCEEDED,
  HWSWITCH_STATE_UPDATE_FAILED,
  HWSWITCH_STATE_UPDATE_CANCELLED,
};

enum HwSwitchOperDeltaSyncState {
  DISCONNECTED, /* initial state */
  WAITING_INITIAL_SYNC, /* Got first getOper request */
  INITIAL_OPER_SENT, /* waiting for initial sync response */
  OPER_SYNCED, /* initial sync completed */
  CANCELLED, /* cancelled */
};

using HwSwitchStateUpdateResult =
    std::pair<std::shared_ptr<SwitchState>, HwSwitchStateUpdateStatus>;

using HwSwitchStateOperUpdateResult =
    std::pair<fsdb::OperDelta, HwSwitchStateUpdateStatus>;

class HwSwitchHandler {
 public:
  HwSwitchHandler(const SwitchID& switchId, const cfg::SwitchInfo& info);

  void start();

  virtual ~HwSwitchHandler();

  folly::Future<HwSwitchStateUpdateResult> stateChanged(
      HwSwitchStateUpdate update);

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

  virtual void gracefulExit(state::WarmbootState& thriftSwitchState) = 0;

  virtual bool getAndClearNeighborHit(RouterID vrf, folly::IPAddress& ip) = 0;

  virtual folly::dynamic toFollyDynamic() const = 0;

  virtual std::optional<uint32_t> getHwLogicalPortId(PortID portID) const = 0;

  virtual bool transactionsSupported(
      std::optional<cfg::SdkVersion> sdkVersion) const = 0;

  virtual HwSwitchFb303Stats* getSwitchStats() const = 0;

  virtual folly::F14FastMap<std::string, HwPortStats> getPortStats() const = 0;

  virtual CpuPortStats getCpuPortStats() const = 0;

  virtual std::map<std::string, HwSysPortStats> getSysPortStats() const = 0;

  virtual void updateStats() = 0;

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

  virtual void switchRunStateChanged(SwitchRunState newState) = 0;

  virtual std::shared_ptr<SwitchState> stateChanged(
      const StateDelta& delta,
      bool transaction) = 0;

  virtual HwSwitchStateOperUpdateResult stateChanged(
      const fsdb::OperDelta& delta,
      bool transaction) = 0;

  // platform access apis
  virtual void onHwInitialized(HwSwitchCallback* callback) = 0;

  virtual void onInitialConfigApplied(HwSwitchCallback* sw) = 0;

  virtual void platformStop() = 0;

  virtual std::map<PortID, FabricEndpoint> getFabricReachability() const = 0;

  virtual FabricReachabilityStats getFabricReachabilityStats() const = 0;

  virtual std::vector<PortID> getSwitchReachability(
      SwitchID switchId) const = 0;

  virtual std::string getDebugDump() const = 0;

  virtual void fetchL2Table(std::vector<L2EntryThrift>* l2Table) const = 0;

  virtual std::string listObjects(
      const std::vector<HwObjectType>& types,
      bool cached) const = 0;

  virtual bool needL2EntryForNeighbor(
      const cfg::SwitchConfig* config) const = 0;

  virtual multiswitch::StateOperDelta getNextStateOperDelta(
      std::unique_ptr<multiswitch::StateOperDelta> prevOperResult,
      bool initialSync) = 0;

  virtual void notifyHwSwitchDisconnected() = 0;

  SwitchID getSwitchId() const {
    return switchId_;
  }
  virtual HwSwitchOperDeltaSyncState getHwSwitchOperDeltaSyncState() = 0;

 private:
  HwSwitchStateUpdateResult stateChangedImpl(const HwSwitchStateUpdate& update);

  HwSwitchStateOperUpdateResult stateChangedImpl(
      const fsdb::OperDelta& delta,
      bool transaction);

  void run();

  void stop();

  SwitchID switchId_;
  cfg::SwitchInfo info_;
  folly::EventBase hwSwitchManagerEvb_;
  std::unique_ptr<std::thread> hwSwitchManagerThread_;
  OperDeltaFilter operDeltaFilter_;
};

} // namespace facebook::fboss
