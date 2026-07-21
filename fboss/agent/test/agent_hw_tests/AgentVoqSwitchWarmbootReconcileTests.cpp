// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/DsfStateUpdaterUtil.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/RemoteIntfRouteAuditor.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/rib/FibUpdateHelpers.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.h"
#include "fboss/agent/test/utils/DsfConfigUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

#include <fb303/ThreadCachedServiceData.h>
#include <folly/IPAddress.h>

namespace facebook::fboss {

class AgentVoqSwitchWarmbootReconcileTest : public AgentVoqSwitchTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentVoqSwitchTest::initialConfig(ensemble);
    cfg.dsfNodes() = *utility::addRemoteIntfNodeCfg(*cfg.dsfNodes(), 1);
    return cfg;
  }

  void setCmdLineFlagOverrides() const override {
    AgentVoqSwitchTest::setCmdLineFlagOverrides();
    FLAGS_enable_remote_intf_route_reconcile = true;
  }

 protected:
  using RemoteRifSpec =
      std::tuple<SystemPortID, InterfaceID, Interface::Addresses>;
  void addRemoteRifsViaDsfUpdate(const std::vector<RemoteRifSpec>& specs) {
    auto remoteSwitchId = utility::getRemoteVoqSwitchId(getSw());
    auto sysPorts = std::make_shared<SystemPortMap>();
    auto rifs = std::make_shared<InterfaceMap>();
    for (const auto& [sysPortId, intfId, addrs] : specs) {
      sysPorts->addNode(utility::makeRemoteSysPort(sysPortId, remoteSwitchId));
      rifs->addNode(utility::makeRemoteInterface(intfId, addrs));
    }
    std::map<SwitchID, std::shared_ptr<SystemPortMap>> switchId2SysPorts{
        {remoteSwitchId, sysPorts}};
    std::map<SwitchID, std::shared_ptr<InterfaceMap>> switchId2Rifs{
        {remoteSwitchId, rifs}};

    auto* sw = getSw();
    sw->getRib()->updateStateInRibThread(
        [sw, switchId2SysPorts, switchId2Rifs]() {
          sw->updateStateWithHwFailureProtection(
              "add remote rifs via dsf for warmboot reconcile test",
              [sw, switchId2SysPorts, switchId2Rifs](
                  const std::shared_ptr<SwitchState>& in) {
                return DsfStateUpdaterUtil::getUpdatedState(
                    in,
                    sw->getScopeResolver(),
                    sw->getRib(),
                    switchId2SysPorts,
                    switchId2Rifs);
              });
        });
  }

  // True if the FIB has an exact-match REMOTE_INTERFACE_ROUTE for `prefix`
  // pointing at `intfId`.
  bool fibHasRoute(
      const std::shared_ptr<SwitchState>& state,
      const folly::CIDRNetwork& prefix,
      const InterfaceID& intfId) {
    auto fib =
        state->getFibsInfoMap()->getFibContainer(RouterID(0))->getFibV6();
    auto route =
        fib->exactMatch(RoutePrefixV6{prefix.first.asV6(), prefix.second});
    if (!route) {
      return false;
    }
    auto entry = route->getEntryForClient(ClientID::REMOTE_INTERFACE_ROUTE);
    if (!entry) {
      return false;
    }
    // Use the ID-aware resolver rather than reading inline nexthops off the
    // entry directly (see FibHelpers::getNextHops / RouteNextHopEntry).
    auto nhSet = getNextHops(state, *entry);
    return nhSet.size() == 1 && nhSet.begin()->intf() == intfId;
  }

  bool fibHasPrefix(
      const std::shared_ptr<SwitchState>& state,
      const folly::CIDRNetwork& prefix) {
    auto fib =
        state->getFibsInfoMap()->getFibContainer(RouterID(0))->getFibV6();
    return fib->exactMatch(RoutePrefixV6{prefix.first.asV6(), prefix.second}) !=
        nullptr;
  }

  // TLTimeseries values are buffered per-thread; flush before reading.
  int64_t inconsistencyCounter() {
    fb303::ThreadCachedServiceData::get()->publishStats();
    return getSw()->stats()->getWarmbootRemoteIntfRoutesInconsistency();
  }
};

// "missing" drift: RIF is in state but the connected /127 route is missing
// from the FIB. Reconcile-on-warmboot must (re-)install the route.
TEST_F(AgentVoqSwitchWarmbootReconcileTest, WarmbootReconcileMissingRoute) {
  const auto kMissingPrefix =
      folly::IPAddress::createNetwork("2001:db8:1::/127");
  auto setup = [this, kMissingPrefix]() {
    const auto kSysPortId =
        utility::getRemoteSysPortId(getSw(), getProgrammedState());
    const auto kIntfId = utility::getRemoteIntfId(kSysPortId);
    auto remoteSwitchId = utility::getRemoteVoqSwitchId(getSw());
    // Add only the remote sys port via DSF; then add the RIF directly via
    // state mutation (no rib->updateRemoteInterfaceRoutes call) so the FIB
    // entry never gets programmed — produces a "missing" drift.
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteSysPort(
          in, scopeResolver(), kSysPortId, remoteSwitchId);
    });
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return utility::addRemoteInterface(
          in,
          scopeResolver(),
          kIntfId,
          {{folly::IPAddress(kMissingPrefix.first), kMissingPrefix.second}});
    });
    // Sanity-assert drift before warmboot.
    auto audit = auditRemoteInterfaceRoutes(getProgrammedState());
    EXPECT_FALSE(audit.noMismatches());
    EXPECT_EQ(audit.missing.size(), 1);
  };
  auto verifyPostWarmboot = [this, kMissingPrefix]() {
    const auto kIntfId = utility::getRemoteIntfId(
        utility::getRemoteSysPortId(getSw(), getProgrammedState()));
    // Reconcile runs inline during SwSwitch::init on warm boot, so by the
    // time verifyPostWarmboot is invoked the state is already converged.
    auto state = getProgrammedState();
    EXPECT_TRUE(fibHasRoute(state, kMissingPrefix, kIntfId));
    EXPECT_TRUE(auditRemoteInterfaceRoutes(state).noMismatches());
    EXPECT_EQ(inconsistencyCounter(), 1);
  };
  verifyAcrossWarmBoots(setup, []() {}, []() {}, verifyPostWarmboot);
}

// "extra" drift: FIB has a REMOTE_INTERFACE_ROUTE entry whose
// (intfId, prefix) tuple has no matching RIF in state. Reconcile must remove
// it.
TEST_F(AgentVoqSwitchWarmbootReconcileTest, WarmbootReconcileExtraRoute) {
  const auto kPhantomPrefix =
      folly::IPAddress::createNetwork("2001:db8:dead::/127");
  const auto kPhantomIntfId = InterfaceID(99999);
  auto setup = [this, kPhantomPrefix, kPhantomIntfId]() {
    // Baseline: legitimate RIF + route via DSF helper, so audit can later
    // distinguish phantom-only drift from a fully-broken setup.
    const auto kSysPortId =
        utility::getRemoteSysPortId(getSw(), getProgrammedState());
    const auto kIntfId = utility::getRemoteIntfId(kSysPortId);
    addRemoteRifsViaDsfUpdate(
        {{kSysPortId, kIntfId, {{folly::IPAddress("2001:db8:2::"), 127}}}});
    // Inject a phantom REMOTE_INTERFACE_ROUTE for an intfId that doesn't
    // exist as a RIF — pure "extra" drift.
    applyNewState([this, kPhantomPrefix, kPhantomIntfId](
                      const std::shared_ptr<SwitchState>& in) {
      auto out = in;
      RoutingInformationBase::RouterIDAndNetworkToInterfaceRoutes toAdd;
      toAdd[RouterID(0)][kPhantomPrefix] = std::make_pair(
          kPhantomIntfId, folly::IPAddress(kPhantomPrefix.first));
      boost::container::flat_map<
          RouterID,
          std::vector<std::pair<folly::CIDRNetwork, InterfaceID>>>
          toDel;
      getSw()->getRib()->updateRemoteInterfaceRoutes(
          getSw()->getScopeResolver(),
          toAdd,
          toDel,
          &ribToSwitchStateUpdate,
          static_cast<void*>(&out));
      return out;
    });
    auto audit = auditRemoteInterfaceRoutes(getProgrammedState());
    EXPECT_FALSE(audit.noMismatches());
    ASSERT_EQ(audit.extra.size(), 1);
    ASSERT_EQ(audit.extra.at(RouterID(0)).size(), 1);
    EXPECT_EQ(
        audit.extra.at(RouterID(0)).front(),
        std::make_pair(kPhantomPrefix, kPhantomIntfId));
  };
  auto verifyPostWarmboot = [this, kPhantomPrefix]() {
    // Reconcile runs inline during SwSwitch::init on warm boot, so by the
    // time verifyPostWarmboot is invoked the state is already converged.
    auto state = getProgrammedState();
    EXPECT_FALSE(fibHasPrefix(state, kPhantomPrefix));
    EXPECT_TRUE(auditRemoteInterfaceRoutes(state).noMismatches());
    EXPECT_EQ(inconsistencyCounter(), 1);
  };
  verifyAcrossWarmBoots(setup, []() {}, []() {}, verifyPostWarmboot);
}

} // namespace facebook::fboss
