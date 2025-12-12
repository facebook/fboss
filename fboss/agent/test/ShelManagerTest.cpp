// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/ShelManager.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

namespace {
constexpr auto kLocalSysPortMax = 20;
constexpr auto kRemoteSysPortMax = 40;
constexpr auto kEcmpWidth = 4;
constexpr auto kEcmpWeight = 1;
} // namespace

namespace facebook::fboss {

class ShelManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    shelManager_ = std::make_unique<ShelManager>();
  }

  std::shared_ptr<SwitchState> switchStateWithLocalSysPorts(
      int localSysPortMax) {
    std::shared_ptr<SwitchState> switchState = std::make_shared<SwitchState>();
    switchState->resetSystemPorts(multiSwitchSysPortMap(0, localSysPortMax));
    return switchState;
  }

  std::shared_ptr<SwitchState> switchStateWithLocalAndRemoteSysPorts(
      int localSysPortMax,
      int remoteSysPortMax) {
    std::shared_ptr<SwitchState> switchState = std::make_shared<SwitchState>();
    switchState->resetSystemPorts(multiSwitchSysPortMap(0, localSysPortMax));
    switchState->resetRemoteSystemPorts(
        multiSwitchSysPortMap(localSysPortMax, remoteSysPortMax));
    return switchState;
  }

  std::shared_ptr<SwitchState> switchStateWithEcmpRoute(
      int localSysPortMax,
      int remoteSysPortMax,
      bool routeResolvedToLocalPorts) {
    auto initialState = switchStateWithLocalAndRemoteSysPorts(
        localSysPortMax, remoteSysPortMax);

    // Add ports to the state and set desiredSelfHealingECMPLagEnable to true
    auto portMap = initialState->getPorts()->clone();
    for (int i = 0; i < localSysPortMax; ++i) {
      state::PortFields portFields;
      portFields.portId() = PortID(i + 1);
      portFields.portName() = fmt::format("port{}", i + 1);
      auto port = std::make_shared<Port>(std::move(portFields));
      port->setPortType(cfg::PortType::INTERFACE_PORT);
      port->setAdminState(cfg::PortState::ENABLED);
      port->setDesiredSelfHealingECMPLagEnable(true);
      portMap->addNode(port, HwSwitchMatcher());
    }
    initialState->resetPorts(portMap);

    // Prepare new state by adding a route with ECMP next-hops
    auto newState = initialState->clone();
    std::vector<ResolvedNextHop> ecmpNexthops;
    ecmpNexthops.reserve(kEcmpWidth);
    int intfOffset = routeResolvedToLocalPorts ? 0 : localSysPortMax;
    for (int i = 0; i < kEcmpWidth; ++i) {
      ecmpNexthops.emplace_back(
          folly::IPAddress(folly::to<std::string>("10.0.0.", i + 1)),
          InterfaceID(i + 1 + intfOffset),
          kEcmpWeight);
    }
    RouteNextHopSet ecmpNextHopSet(ecmpNexthops.begin(), ecmpNexthops.end());

    // Add route to FIB
    RoutePrefixV4 v4Prefix{folly::IPAddressV4("10.0.0.0"), 24};
    auto v4Route = std::make_shared<RouteV4>(RouteV4::makeThrift(v4Prefix));
    auto entry = RouteNextHopEntry(ecmpNextHopSet, AdminDistance::EBGP);
    v4Route->setResolved(entry);
    v4Route->publish();

    auto fibContainer =
        std::make_shared<ForwardingInformationBaseContainer>(RouterID(0));
    ForwardingInformationBaseV4 fibV4;
    fibV4.addNode(v4Route);
    fibContainer->setFib(fibV4.clone());

    auto fibMap = std::make_shared<MultiSwitchForwardingInformationBaseMap>();
    fibMap->addNode(fibContainer, HwSwitchMatcher());
    newState->resetForwardingInformationBases(fibMap);

    auto multiSwitchSwitchSettings = std::make_shared<MultiSwitchSettings>();
    auto addSwitchSettings = [&multiSwitchSwitchSettings,
                              localSysPortMax](SwitchID switchId) {
      auto newSwitchSettings = std::make_shared<SwitchSettings>();
      newSwitchSettings->setSwitchIdToSwitchInfo({std::make_pair(
          switchId,
          createSwitchInfo(
              cfg::SwitchType::VOQ,
              cfg::AsicType::ASIC_TYPE_MOCK,
              cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN(),
              cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX(),
              0,
              0,
              localSysPortMax))});
      multiSwitchSwitchSettings->addNode(
          HwSwitchMatcher(std::unordered_set<SwitchID>({switchId}))
              .matcherString(),
          newSwitchSettings);
    };
    addSwitchSettings(SwitchID(0));
    newState->resetSwitchSettings(std::move(multiSwitchSwitchSettings));
    return newState;
  }

 protected:
  std::shared_ptr<MultiSwitchSystemPortMap> multiSwitchSysPortMap(
      int sysPortMin,
      int sysPortMax) {
    std::shared_ptr<MultiSwitchSystemPortMap> multiSwitchSysPorts =
        std::make_shared<MultiSwitchSystemPortMap>();
    for (int i = sysPortMin; i < sysPortMax; i++) {
      auto sysPort = std::make_shared<SystemPort>(SystemPortID(i + 1));
      sysPort->setScope(cfg::Scope::GLOBAL);
      multiSwitchSysPorts->addNode(sysPort, HwSwitchMatcher());
    }
    return multiSwitchSysPorts;
  }

  std::unique_ptr<ShelManager> shelManager_;
};

TEST_F(ShelManagerTest, RefCountAndIntf2AddDel) {
  auto ecmpWidth = 4;
  auto ecmpWeight = 1;
  std::vector<ResolvedNextHop> ecmpNexthops;
  ecmpNexthops.reserve(ecmpWidth);
  for (int i = 0; i < ecmpWidth; i++) {
    ecmpNexthops.emplace_back(
        folly::IPAddress(folly::to<std::string>("1.1.1.", i + 1)),
        InterfaceID(i + 1),
        ecmpWeight);
  }
  RouteNextHopSet ecmpNextHopSet(ecmpNexthops.begin(), ecmpNexthops.end());
  auto switchState = switchStateWithLocalSysPorts(kLocalSysPortMax);

  shelManager_->updateRefCount(ecmpNextHopSet, switchState, true /*add*/);
  EXPECT_EQ(shelManager_->intf2RefCnt_.rlock()->size(), ecmpWidth);
  for (int i = 0; i < ecmpWidth; i++) {
    EXPECT_EQ(shelManager_->intf2RefCnt_.rlock()->at(InterfaceID(i + 1)), 1);
  }

  shelManager_->updateRefCount(ecmpNextHopSet, switchState, false /*add*/);
  EXPECT_TRUE(shelManager_->intf2RefCnt_.rlock()->empty());
}

TEST_F(ShelManagerTest, EcmpOverShelDisabledPort) {
  std::vector<ResolvedNextHop> ecmpNexthops;
  ecmpNexthops.reserve(kEcmpWidth);
  for (int i = 0; i < kEcmpWidth; i++) {
    ecmpNexthops.emplace_back(
        folly::IPAddress(folly::to<std::string>("1.1.1.", i + 1)),
        InterfaceID(i + 1),
        kEcmpWeight);
  }
  RouteNextHopSet ecmpNextHopSet(ecmpNexthops.begin(), ecmpNexthops.end());
  auto switchState = switchStateWithLocalSysPorts(kLocalSysPortMax);

  // Update refCount for several interfaces
  shelManager_->updateRefCount(ecmpNextHopSet, switchState, true /*add*/);

  std::map<int, cfg::PortState> allEnabledSysPortShelState;
  std::map<int, cfg::PortState> halfDisabledSysPortShelState;
  for (int i = 0; i < kEcmpWidth; i++) {
    allEnabledSysPortShelState[i + 1] = cfg::PortState::ENABLED;
    halfDisabledSysPortShelState[i + 1] =
        (i % 2 == 0) ? cfg::PortState::DISABLED : cfg::PortState::ENABLED;
  }

  // Validate ecmpOverShelDisabledPort
  EXPECT_FALSE(
      shelManager_->ecmpOverShelDisabledPort(allEnabledSysPortShelState));
  EXPECT_TRUE(
      shelManager_->ecmpOverShelDisabledPort(halfDisabledSysPortShelState));

  // Remove refCount and validate again
  shelManager_->updateRefCount(ecmpNextHopSet, switchState, false /*add*/);
  EXPECT_FALSE(
      shelManager_->ecmpOverShelDisabledPort(allEnabledSysPortShelState));
  EXPECT_FALSE(
      shelManager_->ecmpOverShelDisabledPort(halfDisabledSysPortShelState));
}

TEST_F(ShelManagerTest, EnableShelPortWithLocalEcmp) {
  // Create StateDelta
  std::vector<facebook::fboss::StateDelta> deltas;
  auto initialState = switchStateWithLocalSysPorts(kLocalSysPortMax);
  auto newState = switchStateWithEcmpRoute(
      kLocalSysPortMax, kRemoteSysPortMax, true /*routeResolvedToLocalPorts*/);
  deltas.emplace_back(initialState, newState);

  auto retDeltas = shelManager_->modifyState(deltas);

  // Verify the modified state has SelfHealingECMPLagEnable set for ECMP ports
  ASSERT_EQ(retDeltas.size(), 1);
  auto modifiedState = retDeltas.begin()->newState();
  for (int i = 0; i < kLocalSysPortMax; ++i) {
    auto portId = PortID(i + 1);
    auto port = modifiedState->getPorts()->getNodeIf(portId);
    ASSERT_NE(port, nullptr);
    if (i < kEcmpWidth) {
      EXPECT_TRUE(port->getSelfHealingECMPLagEnable().has_value());
      EXPECT_TRUE(port->getSelfHealingECMPLagEnable().value());
    } else {
      EXPECT_FALSE(port->getSelfHealingECMPLagEnable().has_value());
    }
  }
}

TEST_F(ShelManagerTest, EnableShelPortWithRemoteEcmp) {
  // Create StateDelta
  std::vector<facebook::fboss::StateDelta> deltas;
  auto initialState = switchStateWithLocalAndRemoteSysPorts(
      kLocalSysPortMax, kRemoteSysPortMax);
  auto newState = switchStateWithEcmpRoute(
      kLocalSysPortMax, kRemoteSysPortMax, false /*routeResolvedToLocalPorts*/);
  deltas.emplace_back(initialState, newState);

  auto retDeltas = shelManager_->modifyState(deltas);

  // Verify the modified state has SelfHealingECMPLagEnable set for ECMP ports
  ASSERT_EQ(retDeltas.size(), 1);
  auto modifiedState = retDeltas.begin()->newState();
  for (int i = 0; i < kLocalSysPortMax; ++i) {
    auto portId = PortID(i + 1);
    auto port = modifiedState->getPorts()->getNodeIf(portId);
    ASSERT_NE(port, nullptr);
    EXPECT_FALSE(port->getSelfHealingECMPLagEnable().has_value());
  }
}

TEST_F(ShelManagerTest, ShelDownOnLocalPort) {
  // Create StateDelta
  std::vector<facebook::fboss::StateDelta> deltas;
  auto initialState = switchStateWithLocalSysPorts(kLocalSysPortMax);
  auto newState = switchStateWithEcmpRoute(
      kLocalSysPortMax, kRemoteSysPortMax, true /*routeResolvedToLocalPorts*/);
  deltas.emplace_back(initialState, newState);

  auto retDeltas = shelManager_->modifyState(deltas);

  std::map<int, cfg::PortState> enabledShelState;
  std::map<int, cfg::PortState> disabledShelState;
  for (int i = 0; i < kLocalSysPortMax; i++) {
    enabledShelState[i + 1] = cfg::PortState::ENABLED;
    disabledShelState[i + 1] = cfg::PortState::DISABLED;
  }

  EXPECT_FALSE(shelManager_->ecmpOverShelDisabledPort(enabledShelState));
  EXPECT_TRUE(shelManager_->ecmpOverShelDisabledPort(disabledShelState));
}

TEST_F(ShelManagerTest, ShelDownOnRemotePort) {
  // Create StateDelta
  std::vector<facebook::fboss::StateDelta> deltas;
  auto initialState = switchStateWithLocalAndRemoteSysPorts(
      kLocalSysPortMax, kRemoteSysPortMax);
  auto newState = switchStateWithEcmpRoute(
      kLocalSysPortMax, kRemoteSysPortMax, false /*routeResolvedToLocalPorts*/);
  deltas.emplace_back(initialState, newState);

  auto retDeltas = shelManager_->modifyState(deltas);

  std::map<int, cfg::PortState> enabledShelState;
  std::map<int, cfg::PortState> disabledShelState;
  for (int i = kLocalSysPortMax; i < kRemoteSysPortMax; i++) {
    enabledShelState[i + 1] = cfg::PortState::ENABLED;
    disabledShelState[i + 1] = cfg::PortState::DISABLED;
  }

  EXPECT_FALSE(shelManager_->ecmpOverShelDisabledPort(enabledShelState));
  EXPECT_TRUE(shelManager_->ecmpOverShelDisabledPort(disabledShelState));
}
} // namespace facebook::fboss
