// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/hw_test/SaiLinkStateDependentTests.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"

using namespace ::facebook::fboss;

class SaiNextHopGroupTest : public SaiLinkStateDependentTests {
 public:
  void SetUp() override {
    SaiLinkStateDependentTests::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(
        getHwSwitch(),
        {masterLogicalPortIds()[0],
         masterLogicalPortIds()[1],
         masterLogicalPortIds()[2],
         masterLogicalPortIds()[3]});
  }

  void addRoute(int nextHopGroupMemberCount) {
    applyNewState(helper_->setupECMPForwarding(
        getProgrammedState(), nextHopGroupMemberCount));
  }

  void resolveNeighbors(int neighborCount) {
    applyNewState(
        helper_->resolveNextHops(getProgrammedState(), neighborCount));
  }

  void unresolveNeighbors(int neighborCount) {
    applyNewState(
        helper_->unresolveNextHops(getProgrammedState(), neighborCount));
  }

  SaiRouteTraits::RouteEntry routeEntry() const {
    const auto* managerTable = getSaiSwitch()->managerTable();
    const auto* vrHandle =
        managerTable->virtualRouterManager().getVirtualRouterHandle(
            RouterID(0));
    return SaiRouteTraits::RouteEntry{getSaiSwitch()->getSwitchId(),
                                      vrHandle->virtualRouter->adapterKey(),
                                      folly::CIDRNetwork("::", 0)};
  }

  int nextHopGroupActiveSubscriberCount() const {
    const auto* managerTable = getSaiSwitch()->managerTable();
    const auto* routeHandle =
        managerTable->routeManager().getRouteHandle(routeEntry());
    const auto& members = routeHandle->nextHopGroupHandle()->members_;
    return std::count_if(
        members.begin(), members.end(), [](const auto& member) {
          return member->isAlive();
        });
  }

  int nextHopGroupMemberCount() const {
    const auto* managerTable = getSaiSwitch()->managerTable();
    const auto* routeHandle =
        managerTable->routeManager().getRouteHandle(routeEntry());
    const auto nextHopGroupHandle = routeHandle->nextHopGroupHandle();
    auto adapterKey = nextHopGroupHandle->nextHopGroup->adapterKey();

    auto& api = SaiApiTable::getInstance()->nextHopGroupApi();
    auto memberList = api.getAttribute(
        adapterKey,
        SaiNextHopGroupTraits::Attributes::NextHopMemberList{
            std::vector<sai_object_id_t>(1)});
    return memberList.size();
  }

  void verifyMemberCount(int count) {
    EXPECT_EQ(nextHopGroupActiveSubscriberCount(), count);
    EXPECT_EQ(nextHopGroupMemberCount(), count);
  }

 protected:
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

TEST_F(SaiNextHopGroupTest, addNextHopGroupWithUnresolvedNeighbors) {
  auto setup = [=]() { addRoute(4); };
  auto verify = [=]() { verifyMemberCount(0); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiNextHopGroupTest, addNextHopGroupWithResolvedNeighbors) {
  auto setup = [=]() {
    resolveNeighbors(4);
    addRoute(4);
  };
  auto verify = [=]() { verifyMemberCount(4); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiNextHopGroupTest, addNextHopGroupThenResolveAll) {
  auto setup = [=]() {
    addRoute(4);
    resolveNeighbors(4);
  };
  auto verify = [=]() { verifyMemberCount(4); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiNextHopGroupTest, addNextHopGroupThenUnresolveAll) {
  auto setup = [=]() {
    resolveNeighbors(4);
    addRoute(4);
    unresolveNeighbors(4);
  };
  auto verify = [=]() { verifyMemberCount(0); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiNextHopGroupTest, addNextHopGroupThenUnresolveSome) {
  auto setup = [=]() {
    resolveNeighbors(4);
    addRoute(4);
    unresolveNeighbors(2);
  };
  auto verify = [=]() { verifyMemberCount(2); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiNextHopGroupTest, addNextHopGroupPortDown) {
  auto setup = [=]() {
    resolveNeighbors(4);
    addRoute(4);
    bringDownPort(masterLogicalPortIds()[0]);
  };
  auto verify = [=]() { verifyMemberCount(3); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiNextHopGroupTest, addNextHopGroupPortDownPortUp) {
  auto setup = [=]() {
    resolveNeighbors(4);
    addRoute(4);
    bringDownPort(masterLogicalPortIds()[0]);
    unresolveNeighbors(1);
    bringUpPort(masterLogicalPortIds()[0]);
    resolveNeighbors(1);
  };
  auto verify = [=]() { verifyMemberCount(4); };
  verifyAcrossWarmBoots(setup, verify);
}
