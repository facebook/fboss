#pragma once

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/hw/bcm/BcmTxPacket.h"
#include "fboss/agent/hw/bcm/gen-cpp2/packettrace_types.h"

#include <folly/Optional.h>
#include <gmock/gmock.h>

namespace facebook {
namespace fboss {

class MockBcmSwitch : public BcmSwitchIf {
 public:
  MockBcmSwitch() {}

  MOCK_METHOD0(releaseUnit, std::unique_ptr<BcmUnit>());
  MOCK_METHOD0(resetTables, void());
  MOCK_METHOD1(initTables, void(const folly::dynamic&));
  MOCK_CONST_METHOD0(getPlatform, BcmPlatform*());
  MOCK_METHOD1(init, HwInitResult(Callback* callback));
  MOCK_METHOD0(unregisterCallbacks, void());
  MOCK_METHOD1(allocatePacket, std::unique_ptr<TxPacket>(uint32_t size));
  // hackery, since GMOCK does no support forwarding
  bool sendPacketSwitchedAsync(
      std::unique_ptr<TxPacket> pkt) noexcept override {
    return sendPacketSwitchedAsyncImpl(pkt.get());
  }
  GMOCK_METHOD1_(, noexcept, , sendPacketSwitchedAsyncImpl,
      bool(TxPacket* pkt));
  // ditto for the other method
  bool sendPacketOutOfPortAsync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID,
      folly::Optional<uint8_t> cos = folly::none) noexcept override {
    return sendPacketOutOfPortAsyncImpl(pkt.get(), portID, cos);
  }
  // hackery, since GMOCK does no support forwarding
  bool sendPacketSwitchedSync(std::unique_ptr<TxPacket> pkt) noexcept override {
    return sendPacketSwitchedAsyncImpl(pkt.get());
  }
  // ditto for the other method
  bool sendPacketOutOfPortSync(
      std::unique_ptr<TxPacket> pkt,
      PortID portID) noexcept override {
    return sendPacketOutOfPortAsyncImpl(pkt.get(), portID, folly::none);
  }
  GMOCK_METHOD3_(
      ,
      noexcept,
      ,
      sendPacketOutOfPortAsyncImpl,
      bool(TxPacket* pkt, PortID portID, folly::Optional<uint8_t> cos));
  // and another one
  std::unique_ptr<PacketTraceInfo> getPacketTrace(
      std::unique_ptr<MockRxPacket> pkt) override {
    return getPacketTraceImpl(pkt.get());
  }
  MOCK_METHOD1(
      getPacketTraceImpl,
      std::unique_ptr<PacketTraceInfo>(MockRxPacket* pkt));
  MOCK_METHOD0(isRxThreadRunning, bool());
  MOCK_CONST_METHOD0(getUnit, int());
  MOCK_CONST_METHOD0(getPortTable, const BcmPortTable*());
  MOCK_CONST_METHOD0(getIntfTable, const BcmIntfTable*());
  MOCK_CONST_METHOD0(getHostTable, const BcmHostTable*());
  MOCK_CONST_METHOD0(getAclTable, const BcmAclTable*());
  MOCK_CONST_METHOD0(getQosPolicyTable, const BcmQosPolicyTable*());
  MOCK_CONST_METHOD0(getStatUpdater, BcmStatUpdater*());
  MOCK_CONST_METHOD0(getTrunkTable, const BcmTrunkTable*());
  MOCK_CONST_METHOD1(isPortUp, bool(PortID port));
  MOCK_CONST_METHOD0(getDropEgressId, opennsl_if_t());
  MOCK_CONST_METHOD0(getToCPUEgressId, opennsl_if_t());
  MOCK_METHOD1(
      stateChanged,
      std::shared_ptr<SwitchState>(const StateDelta& delta));
  MOCK_METHOD1(gracefulExit, void(folly::dynamic& switchState));
  MOCK_CONST_METHOD0(toFollyDynamic, folly::dynamic());
  MOCK_METHOD0(initialConfigApplied, void());
  MOCK_METHOD0(clearWarmBootCache, void());
  MOCK_METHOD1(updateStats, void(SwitchStats* switchStats));
  MOCK_CONST_METHOD0(getCosMgr, BcmCosManager*());
  MOCK_METHOD1(fetchL2Table, void(std::vector<L2EntryThrift>* l2Table));
  MOCK_CONST_METHOD0(writableHostTable, BcmHostTable*());
  MOCK_CONST_METHOD0(writableAclTable, BcmAclTable*());
  MOCK_CONST_METHOD0(getWarmBootCache, BcmWarmBootCache*());
  MOCK_CONST_METHOD1(dumpState, void(const std::string& path));
  MOCK_CONST_METHOD0(gatherSdkState, std::string());
  MOCK_CONST_METHOD0(exitFatal, void());
  MOCK_METHOD2(
      getAndClearNeighborHit,
      bool(RouterID vrf, folly::IPAddress& ip));
  MOCK_CONST_METHOD1(isValidStateUpdate, bool(const StateDelta& delta));
  MOCK_CONST_METHOD0(getControlPlane, BcmControlPlane*());
  MOCK_METHOD1(
      clearPortStats,
      void(const std::unique_ptr<std::vector<int32_t>>&));
  MOCK_CONST_METHOD0(getBcmMirrorTable, const BcmMirrorTable*());
  MOCK_CONST_METHOD0(writableBcmMirrorTable, BcmMirrorTable*());
};

} // namespace fboss
} // namespace facebook
