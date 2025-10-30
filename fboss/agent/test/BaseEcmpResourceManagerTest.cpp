/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/BaseEcmpResourceManagerTest.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/SwSwitchWarmBootHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/EcmpResourceManagerTestUtils.h"

#include <functional>

namespace facebook::fboss {

RouteNextHopSet makeNextHops(int n, int numNhopsPerIntf, int startOffset) {
  RouteNextHopSet h;
  for (int i = 0; i < n; i++) {
    auto interfaceId = (i / numNhopsPerIntf) + 1;
    auto subnetIndex = i / numNhopsPerIntf;
    std::stringstream ss;
    ss << std::hex << subnetIndex;
    // ::1 is local interface address, start nhop addressed from 2
    auto lastQuad = 2 + startOffset + i % numNhopsPerIntf;
    auto ipStr = folly::sformat("2400:db00:2110:{}::{}", ss.str(), lastQuad);
    h.emplace(ResolvedNextHop(
        folly::IPAddress(ipStr),
        InterfaceID(interfaceId),
        UCMP_DEFAULT_WEIGHT));
  }
  return h;
}
RouteNextHopSet makeV4NextHops(int n) {
  CHECK_LT(n, 253);
  RouteNextHopSet h;
  for (int i = 0; i < n; i++) {
    auto ipStr = folly::sformat("200.0.{}.2", i);
    h.emplace(ResolvedNextHop(
        folly::IPAddress(ipStr), InterfaceID(i + 1), UCMP_DEFAULT_WEIGHT));
  }
  return h;
}

RouteV6::Prefix makePrefix(int offset) {
  std::stringstream ss;
  ss << std::hex << offset;
  return RouteV6::Prefix(
      folly::IPAddressV6(folly::sformat("2601:db00:2110:{}::", ss.str())), 64);
}

RouteV4::Prefix makeV4Prefix(int offset) {
  std::stringstream ss;
  ss << std::hex << offset;
  return RouteV4::Prefix(
      folly::IPAddressV4(folly::sformat("150.0.{}.0", ss.str())), 24);
}

std::shared_ptr<RouteV6> makeRoute(
    const RouteV6::Prefix& pfx,
    const RouteNextHopSet& nextHops) {
  RouteNextHopEntry nhopEntry(nextHops, kDefaultAdminDistance);
  auto rt =
      std::make_shared<RouteV6>(RouteV6::makeThrift(pfx, kClientID, nhopEntry));
  rt->setResolved(nhopEntry);
  rt->publish();
  return rt;
}

std::shared_ptr<RouteV4> makeV4Route(
    const RouteV4::Prefix& pfx,
    const RouteNextHopSet& nextHops) {
  RouteNextHopEntry nhopEntry(nextHops, kDefaultAdminDistance);
  auto rt =
      std::make_shared<RouteV4>(RouteV4::makeThrift(pfx, kClientID, nhopEntry));
  rt->setResolved(nhopEntry);
  rt->publish();
  return rt;
}

cfg::SwitchConfig onePortPerIntfConfig(
    int numIntfs,
    std::optional<cfg::SwitchingMode> backupSwitchingMode,
    int32_t ecmpCompressionThresholdPct) {
  cfg::SwitchConfig cfg;
  cfg.ports()->resize(numIntfs);
  cfg.vlans()->resize(numIntfs);
  cfg.interfaces()->resize(numIntfs);
  int idBegin = 1;
  for (int p = 0; p < numIntfs; ++p) {
    auto id = p + idBegin;
    cfg.ports()[p].logicalID() = id;
    cfg.ports()[p].name() = folly::to<std::string>("port", id);
    cfg.ports()[p].speed() = cfg::PortSpeed::TWENTYFIVEG;
    cfg.ports()[p].profileID() =
        cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER;
    cfg.vlans()[p].id() = id;
    cfg.vlans()[p].name() = folly::to<std::string>("Vlan", id);
    cfg.vlans()[p].intfID() = id;
    cfg.interfaces()[p].intfID() = id;
    cfg.interfaces()[p].routerID() = 0;
    cfg.interfaces()[p].vlanID() = id;
    cfg.interfaces()[p].name() = folly::to<std::string>("interface", id);
    cfg.interfaces()[p].mac() = "00:02:00:00:00:01";
    cfg.interfaces()[p].mtu() = 9000;
    cfg.interfaces()[p].ipAddresses()->resize(2);
    std::stringstream ss;
    ss << std::hex << p;
    cfg.interfaces()[p].ipAddresses()[0] =
        folly::sformat("2400:db00:2110:{}::1/64", ss.str());
    cfg.interfaces()[p].ipAddresses()[1] = folly::sformat("200.0.{}.1/24", p);
  }
  if (ecmpCompressionThresholdPct) {
    cfg.switchSettings()->ecmpCompressionThresholdPct() =
        ecmpCompressionThresholdPct;
  }
  if (backupSwitchingMode.has_value()) {
    cfg::FlowletSwitchingConfig flowletConfig;
    flowletConfig.backupSwitchingMode() = cfg::SwitchingMode::PER_PACKET_RANDOM;
    cfg.flowletSwitchingConfig() = flowletConfig;
  }
  return cfg;
}

std::shared_ptr<EcmpResourceManager>
BaseEcmpResourceManagerTest::makeResourceMgrWithEcmpLimit(
    int ecmpGroupLimit) const {
  CHECK(!(getEcmpCompressionThresholdPct() && getBackupEcmpSwitchingMode()));
  return getBackupEcmpSwitchingMode()
      ? std::make_shared<EcmpResourceManager>(
            ecmpGroupLimit, getBackupEcmpSwitchingMode())
      : std::make_shared<EcmpResourceManager>(
            ecmpGroupLimit, getEcmpCompressionThresholdPct());
}

std::vector<StateDelta> BaseEcmpResourceManagerTest::consolidate(
    const std::shared_ptr<SwitchState>& state) {
  state->publish();
  XLOG(DBG2) << " Consolidator update start";
  StateDelta delta(state_, state);
  auto deltas = consolidator_->consolidate(delta);
  consolidator_->updateDone();
  if (deltas.size()) {
    XLOG(DBG2) << " Checking deltas, num deltas: " << deltas.size();
    assertDeltasForOverflow(deltas);
    assertResourceMgrCorrectness(*consolidator_, deltas.back().newState());
  }
  XLOG(DBG2) << " Consolidator update done";
  XLOG(DBG2) << " SwSwitch update start";
  updateFlowletSwitchingConfig(state);
  updateRoutes(state);
  assertResourceMgrCorrectness(*sw_->getEcmpResourceManager(), sw_->getState());
  XLOG(DBG2) << " SwSwitch update done";
  CHECK(state_->isPublished());
  state_ = sw_->getState();
  /*
   * GE since some tests add v4 routes
   */
  EXPECT_GE(
      state_->getFibs()->getNode(RouterID(0))->getFibV4()->size(),
      kNumIntfs + 1);
  /*
   * Assert that EcmpResourceMgr leaves the ports state untouched
   */
  EXPECT_NE(state_->getPorts()->getPortIf("port1"), nullptr);
  {
    // Assert restoration from current state results in no
    // overflow and the route backup group state matches
    XLOG(DBG2) << " Asserting for reconstructing from switch state";
    auto newConsolidator = makeResourceMgr();
    auto restoreDeltas = newConsolidator->reconstructFromSwitchState(state_);
    EXPECT_EQ(restoreDeltas.size(), 1);
    auto restoredState = restoreDeltas.back().newState();
    auto newFib6 = cfib(restoredState);
    for (const auto& [_, origRoute] : std::as_const(*cfib(state_))) {
      auto newRoute = newFib6->getRouteIf(origRoute->prefix());
      EXPECT_EQ(newRoute->isResolved(), origRoute->isResolved());
      if (origRoute->isResolved()) {
        EXPECT_EQ(
            newRoute->getForwardInfo().getOverrideEcmpSwitchingMode(),
            origRoute->getForwardInfo().getOverrideEcmpSwitchingMode())
            << " Failed for : " << newRoute->str();
        EXPECT_EQ(
            newRoute->getForwardInfo().getOverrideNextHops(),
            origRoute->getForwardInfo().getOverrideNextHops())
            << " Failed for : " << newRoute->str();
      }
    }
  }
  return deltas;
}
void BaseEcmpResourceManagerTest::failUpdate(
    const std::shared_ptr<SwitchState>& state) {
  failUpdate(state, state_);
}

void BaseEcmpResourceManagerTest::failUpdate(
    const std::shared_ptr<SwitchState>& state,
    const std::shared_ptr<SwitchState>& failTo) {
  StateDelta delta(state_, state);
  auto deltas = consolidator_->consolidate(delta);
  consolidator_->updateFailed(failTo);
}

void BaseEcmpResourceManagerTest::assertDeltasForOverflow(
    const std::vector<StateDelta>& deltas) const {
  facebook::fboss::assertDeltasForOverflow(*consolidator_, deltas);
}

RouteV6::Prefix BaseEcmpResourceManagerTest::nextPrefix() const {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  for (auto offset = 0; offset < std::numeric_limits<uint16_t>::max();
       ++offset) {
    auto pfx = makePrefix(offset);
    if (!fib6->exactMatch(pfx)) {
      XLOG(DBG2) << " Next pfx: " << pfx.str();
      return pfx;
    }
  }
  CHECK(false) << " Should never get here";
}

void BaseEcmpResourceManagerTest::setupFlags() const {
  FLAGS_enable_ecmp_resource_manager = true;
  FLAGS_ecmp_resource_percentage = 100;
  FLAGS_ars_resource_percentage = 100;
  FLAGS_flowletSwitchingEnable = true;
  FLAGS_dlbResourceCheckEnable = false;
}

void BaseEcmpResourceManagerTest::SetUp() {
  XLOG(DBG2) << "BaseEcmpResourceMgrTest SetUp";
  setupFlags();
  auto cfg = onePortPerIntfConfig(
      kNumIntfs,
      getBackupEcmpSwitchingMode(),
      getEcmpCompressionThresholdPct());
  handle_ = createTestHandle(&cfg);
  sw_ = handle_->getSw();
  sw_->initialConfigApplied(std::chrono::steady_clock::now());
  ASSERT_NE(sw_->getEcmpResourceManager(), nullptr);
  // Taken from mock asic
  auto asic = *sw_->getHwAsicTable()->getL3Asics().begin();
  int asicMaxEcmpGroups, maxPct;
  if (getBackupEcmpSwitchingMode()) {
    asicMaxEcmpGroups = *asic->getMaxArsGroups();
    maxPct = FLAGS_ars_resource_percentage;
  } else {
    asicMaxEcmpGroups = *asic->getMaxEcmpGroups();
    maxPct = FLAGS_ecmp_resource_percentage;
  }
  EXPECT_EQ(
      sw_->getEcmpResourceManager()->getMaxPrimaryEcmpGroups(),
      std::floor(asicMaxEcmpGroups * maxPct / 100.0) -
          FLAGS_ecmp_resource_manager_make_before_break_buffer);
  // Backup ecmp group type will come from default flowlet confg
  std::optional<cfg::SwitchingMode> expectedBackupSwitchingMode;
  if (cfg.flowletSwitchingConfig() &&
      cfg.flowletSwitchingConfig()->backupSwitchingMode().has_value()) {
    expectedBackupSwitchingMode =
        *cfg.flowletSwitchingConfig()->backupSwitchingMode();
  }
  EXPECT_EQ(
      sw_->getEcmpResourceManager()->getBackupEcmpSwitchingMode(),
      expectedBackupSwitchingMode);
  EXPECT_EQ(
      sw_->getEcmpResourceManager()->getEcmpCompressionThresholdPct(),
      cfg.switchSettings()->ecmpCompressionThresholdPct().value_or(0));
  consolidator_ = makeResourceMgr();
  state_ = sw_->getState();
  state_->publish();
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  for (auto i = 0; i < numStartRoutes(); ++i) {
    auto pfx = makePrefix(i);
    auto route = makeRoute(pfx, defaultNhops());
    fib6->addNode(pfx.str(), std::move(route));
  }
  newState->publish();
  consolidate(newState);
  XLOG(DBG2) << "BaseEcmpResourceMgrTest SetUp done";
}

void BaseEcmpResourceManagerTest::TearDown() {
  // Assert route replays are noops
  assertReplayIsNoOp(false /*syncFib*/);
  assertReplayIsNoOp(true /*syncFib*/);
  {
    auto wbState = sw_->gracefulExitState();
    auto [reconstructedState, reconstructedRib] =
        sw_->getWarmBootHelper()->reconstructStateAndRib(
            wbState, true /*hasL3*/);
    // Assert reconstructed RIB matches both reconstructed
    // state and current SwitchState. The above APIs will
    // be used on WB to reconstruct both rib and switch
    // state
    facebook::fboss::assertRibFibEquivalence(
        reconstructedState, reconstructedRib.get());
    facebook::fboss::assertRibFibEquivalence(
        sw_->getState(), reconstructedRib.get());
  }
  if (!getEcmpCompressionThresholdPct()) {
    return;
  }
  assertResourceMgrCorrectness(*sw_->getEcmpResourceManager(), sw_->getState());
  assertResourceMgrCorrectness(*consolidator_, state_);
}

void BaseEcmpResourceManagerTest::assertReplayIsNoOp(bool syncFib) {
  auto preAddState = state_;
  ThriftHandler handler(sw_);
  auto replayRoutes = getClientRoutes(kClientID);
  XLOG(DBG2) << " Will replay: " << replayRoutes->size()
             << " routes via: " << (syncFib ? "syncFib" : "addUnicastRoutes");
  if (syncFib) {
    handler.syncFib(static_cast<int16_t>(kClientID), std::move(replayRoutes));
  } else {
    handler.addUnicastRoutes(
        static_cast<int16_t>(kClientID), std::move(replayRoutes));
  }
  state_ = sw_->getState();
  ASSERT_EQ(preAddState, state_);
}

std::shared_ptr<EcmpResourceManager>
BaseEcmpResourceManagerTest::makeResourceMgr() const {
  static constexpr auto kEcmpGroupHwLimit = 100;
  return getBackupEcmpSwitchingMode()
      ? std::make_shared<EcmpResourceManager>(
            kEcmpGroupHwLimit, getBackupEcmpSwitchingMode())
      : std::make_shared<EcmpResourceManager>(
            kEcmpGroupHwLimit, getEcmpCompressionThresholdPct());
}
std::set<EcmpResourceManager::NextHopGroupId>
BaseEcmpResourceManagerTest::getNhopGroupIds() const {
  auto nhop2Id = consolidator_->getNhopsToId();
  std::set<NextHopGroupId> nhopIds;
  std::for_each(
      nhop2Id.begin(), nhop2Id.end(), [&nhopIds](const auto& nhopsAndId) {
        nhopIds.insert(nhopsAndId.second);
      });
  return nhopIds;
}

void BaseEcmpResourceManagerTest::updateFlowletSwitchingConfig(
    const std::shared_ptr<SwitchState>& newState) {
  if (sw_->getState()->getFlowletSwitchingConfig() !=
      newState->getFlowletSwitchingConfig()) {
    auto curConfig = sw_->getConfig();
    if (newState->getFlowletSwitchingConfig()) {
      curConfig.flowletSwitchingConfig()->backupSwitchingMode() =
          newState->getFlowletSwitchingConfig()->getBackupSwitchingMode();
    } else {
      curConfig.flowletSwitchingConfig().reset();
    }
    sw_->applyConfig("flowletSwitching change", curConfig);
  }
}

void BaseEcmpResourceManagerTest::updateRoutes(
    const std::shared_ptr<SwitchState>& newState) {
  StateDelta delta(sw_->getState(), newState);

  auto routesToAddOrUpdate = std::make_unique<std::vector<UnicastRoute>>();
  auto prefixesToDelete = std::make_unique<std::vector<IpPrefix>>();

  processFibsDeltaInHwSwitchOrder(
      delta,
      [&routesToAddOrUpdate](
          RouterID rid, const auto& /*oldRoute*/, const auto& newRoute) {
        routesToAddOrUpdate->emplace_back(
            util::toUnicastRoute(
                newRoute->prefix().toCidrNetwork(),
                newRoute->getForwardInfo()));
      },
      [&routesToAddOrUpdate](RouterID rid, const auto& newRoute) {
        routesToAddOrUpdate->emplace_back(
            util::toUnicastRoute(
                newRoute->prefix().toCidrNetwork(),
                newRoute->getForwardInfo()));
      },
      [&prefixesToDelete](RouterID rid, const auto& oldRoute) {
        IpPrefix pfx;
        pfx.ip() = network::toBinaryAddress(oldRoute->prefix().network());
        pfx.prefixLength() = oldRoute->prefix().mask();
        prefixesToDelete->push_back(pfx);
      });

  ThriftHandler handler(sw_);
  handler.deleteUnicastRoutes(
      static_cast<int16_t>(kClientID), std::move(prefixesToDelete));
  handler.addUnicastRoutes(
      static_cast<int16_t>(kClientID), std::move(routesToAddOrUpdate));
  assertRibFibEquivalence();
}

std::unique_ptr<std::vector<UnicastRoute>>
BaseEcmpResourceManagerTest::getClientRoutes(ClientID client) const {
  auto fibContainer =
      sw_->getState()->getFibs()->getAllNodes()->getFibContainerIf(RouterID(0));
  auto unicastRoutes = std::make_unique<std::vector<UnicastRoute>>();
  auto fillInRoutes = [&unicastRoutes](const auto& fibIn) {
    for (const auto& [_, route] : std::as_const(*fibIn)) {
      auto forwardInfo = route->getEntryForClient(kClientID);
      if (forwardInfo) {
        unicastRoutes->emplace_back(
            util::toUnicastRoute(
                route->prefix().toCidrNetwork(), *forwardInfo));
      }
    }
  };
  fillInRoutes(fibContainer->getFibV4());
  fillInRoutes(fibContainer->getFibV6());
  XLOG(DBG2) << " For client : " << static_cast<int>(client)
             << " got : " << unicastRoutes->size() << " routes";
  return unicastRoutes;
}

void BaseEcmpResourceManagerTest::syncFib() {
  ThriftHandler handler(sw_);
  handler.syncFib(static_cast<int16_t>(kClientID), getClientRoutes(kClientID));
}

void BaseEcmpResourceManagerTest::replayAllRoutesViaThrift() {
  ThriftHandler handler(sw_);
  handler.addUnicastRoutes(
      static_cast<int16_t>(kClientID), getClientRoutes(kClientID));
}

void BaseEcmpResourceManagerTest::assertRibFibEquivalence() const {
  waitForStateUpdates(sw_);
  facebook::fboss::assertRibFibEquivalence(sw_->getState(), sw_->getRib());
}

std::vector<std::shared_ptr<RouteV6>>
BaseEcmpResourceManagerTest::getPostConfigResolvedRoutes(
    const std::shared_ptr<SwitchState>& in) const {
  std::vector<std::shared_ptr<RouteV6>> routes;
  for (const auto& [_, route] : std::as_const(*cfib(in))) {
    if (!route->isResolved() || route->isConnected() ||
        route->getForwardInfo().getNextHopSet().empty()) {
      continue;
    }
    routes.emplace_back(route);
  }
  return routes;
}

std::optional<EcmpResourceManager::NextHopGroupId>
BaseEcmpResourceManagerTest::getNhopId(const RouteNextHopSet& nhops) const {
  std::optional<EcmpResourceManager::NextHopGroupId> nhopId;
  auto nitr = consolidator_->getNhopsToId().find(nhops);
  if (nitr != consolidator_->getNhopsToId().end()) {
    nhopId = nitr->second;
  }
  return nhopId;
}

void BaseEcmpResourceManagerTest::assertTargetState(
    const std::shared_ptr<SwitchState>& targetState,
    const std::shared_ptr<SwitchState>& endStatePrefixes,
    const std::set<RouteV6::Prefix>& overflowPrefixes,
    const EcmpResourceManager* consolidatorToCheck,
    bool checkStats) {
  consolidatorToCheck =
      consolidatorToCheck ? consolidatorToCheck : consolidator_.get();
  /*
   * GE since some tests add v4 routes
   */
  EXPECT_GE(
      state_->getFibs()->getNode(RouterID(0))->getFibV4()->size(),
      kNumIntfs + 1);
  std::set<RouteNextHopSet> primaryEcmpGroups, backupEcmpGroups,
      mergedEcmpGroups;
  EcmpResourceManager::NextHopGroupIds mergedGroupMembers;
  auto checkFib = [&](auto fibToCheck, auto targetStateFib) {
    for (auto [_, inRoute] : std::as_const(*fibToCheck)) {
      auto route = targetStateFib->exactMatch(inRoute->prefix());
      ASSERT_TRUE(route->isResolved());
      ASSERT_NE(route, nullptr);
      auto consolidatorGrpInfo = consolidatorToCheck->getGroupInfo(
          RouterID(0), inRoute->prefix().toCidrNetwork());
      bool isEcmpRoute = route->isResolved() &&
          route->getForwardInfo().getNextHopSet().size() > 1;
      if (isEcmpRoute) {
        ASSERT_NE(consolidatorGrpInfo, nullptr);
        if (!route->getForwardInfo().getOverrideEcmpSwitchingMode()) {
          primaryEcmpGroups.insert(
              route->getForwardInfo().normalizedNextHops());
        }
        if (consolidatorToCheck == consolidator_.get()) {
          /*
           * If consolidatorToCheck is the same as test class consolidator
           * assert that group infos b/w SwSwitch's consolidator_ and
           * Test class consolidator match
           */

          auto swSwitchGroupInfo = sw_->getEcmpResourceManager()->getGroupInfo(
              RouterID(0), route->prefix().toCidrNetwork());
          ASSERT_NE(swSwitchGroupInfo, nullptr);
          auto swGroupId = swSwitchGroupInfo->getID();
          auto consolidatorGroupId = swSwitchGroupInfo->getID();
          auto consolidatorRouteUsageCount =
              consolidatorGrpInfo->getRouteUsageCount();
          auto swRouteUsageCount = swSwitchGroupInfo->getRouteUsageCount();
          auto consolidatorIsBackupEcmpType =
              consolidatorGrpInfo->isBackupEcmpGroupType();
          auto swIsBackupEcmpType = swSwitchGroupInfo->isBackupEcmpGroupType();
          auto consolidatorIsMergeGroup =
              consolidatorGrpInfo->getMergedGroupInfoItr().has_value();
          auto swIsMergeGroup =
              swSwitchGroupInfo->getMergedGroupInfoItr().has_value();
          EXPECT_EQ(
              std::tie(
                  swGroupId,
                  swRouteUsageCount,
                  swIsBackupEcmpType,
                  swIsMergeGroup),
              std::tie(
                  consolidatorGroupId,
                  consolidatorRouteUsageCount,
                  consolidatorIsBackupEcmpType,
                  consolidatorIsMergeGroup));
          // Compare consolidation infos
          auto swMergedGroupConsolidationInfo =
              sw_->getEcmpResourceManager()->getMergeGroupConsolidationInfo(
                  swGroupId);
          auto consolidatorMergedGroupConsolidationInfo =
              consolidatorToCheck->getMergeGroupConsolidationInfo(
                  consolidatorGroupId);
          EXPECT_EQ(
              swMergedGroupConsolidationInfo,
              consolidatorMergedGroupConsolidationInfo);
          auto swCandidateMergeConsolidationInfo =
              sw_->getEcmpResourceManager()->getCandidateMergeConsolidationInfo(
                  swGroupId);
          auto consolidatorCandidateMergeConsolidationInfo =
              consolidatorToCheck->getCandidateMergeConsolidationInfo(
                  consolidatorGroupId);
          EXPECT_EQ(
              swCandidateMergeConsolidationInfo,
              consolidatorCandidateMergeConsolidationInfo);
          if (swMergedGroupConsolidationInfo) {
            ASSERT_TRUE(consolidatorGrpInfo->getMergedGroupInfoItr());
            ASSERT_TRUE(swSwitchGroupInfo->getMergedGroupInfoItr());
            auto swMergeSet =
                (*swSwitchGroupInfo->getMergedGroupInfoItr())->first;
            auto consolidatorMergeSet =
                (*consolidatorGrpInfo->getMergedGroupInfoItr())->first;
            EXPECT_EQ(swMergeSet, consolidatorMergeSet);
            mergedGroupMembers.insert(swMergeSet.begin(), swMergeSet.end());
          }
        }
      }
    }
  };
  auto checkOverflow = [&](const auto fibToCheck, const auto targetStateFib) {
    for (auto [_, inRoute] : std::as_const(*fibToCheck)) {
      auto route = targetStateFib->exactMatch(inRoute->prefix());
      auto consolidatorGrpInfo = consolidatorToCheck->getGroupInfo(
          RouterID(0), inRoute->prefix().toCidrNetwork());
      if (overflowPrefixes.find(route->prefix()) != overflowPrefixes.end()) {
        EXPECT_TRUE(route->getForwardInfo().hasOverrideSwitchingModeOrNhops())
            << " expected route " << route->str()
            << " to have override ECMP group type or ecmp nhops";
        if (getBackupEcmpSwitchingMode()) {
          EXPECT_EQ(
              route->getForwardInfo().getOverrideEcmpSwitchingMode(),
              consolidatorToCheck->getBackupEcmpSwitchingMode());
          EXPECT_TRUE(consolidatorGrpInfo->isBackupEcmpGroupType());
          backupEcmpGroups.insert(route->getForwardInfo().normalizedNextHops());
        }
        if (getEcmpCompressionThresholdPct()) {
          EXPECT_TRUE(
              route->getForwardInfo().getOverrideNextHops().has_value());
          EXPECT_EQ(
              route->getForwardInfo().getOverrideNextHops(),
              consolidatorToCheck
                  ->getGroupInfo(RouterID(0), route->prefix().toCidrNetwork())
                  ->getOverrideNextHops());
          // Merged groups also take up primary ecmp groups
          mergedEcmpGroups.insert(route->getForwardInfo().normalizedNextHops());
        }
      } else {
        EXPECT_FALSE(route->getForwardInfo().hasOverrideSwitchingModeOrNhops())
            << " expected route " << route->str()
            << " to NOT have override ECMP group type or ecmp nhops";
        bool isEcmpRoute = route->isResolved() &&
            route->getForwardInfo().getNextHopSet().size() > 1;
        if (isEcmpRoute) {
          EXPECT_FALSE(consolidatorGrpInfo->isBackupEcmpGroupType());
          EXPECT_FALSE(consolidatorGrpInfo->hasOverrideNextHops());
        }
      }
    }
  };
  checkFib(cfib(endStatePrefixes), cfib(targetState));
  checkFib(cfib4(endStatePrefixes), cfib4(targetState));
  // Overflow prefixes are of v6 type only
  checkOverflow(cfib(endStatePrefixes), cfib(targetState));
  if (checkStats) {
    EXPECT_EQ(
        sw_->stats()->getPrimaryEcmpGroupsExhausted(),
        backupEcmpGroups.size() || mergedEcmpGroups.size() ? 1 : 0);
    EXPECT_EQ(
        sw_->stats()->getPrimaryEcmpGroupsCount(), primaryEcmpGroups.size());
    EXPECT_EQ(
        sw_->stats()->getBackupEcmpGroupsCount(), backupEcmpGroups.size());
    EXPECT_EQ(
        sw_->stats()->getMergedEcmpMemberGroupsCount(),
        mergedGroupMembers.size());
  }
}

std::vector<StateDelta> BaseEcmpResourceManagerTest::addOrUpdateRoutes(
    const std::map<RoutePrefixV6, RouteNextHopSet>& prefix2Nhops) {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  for (const auto& [prefix6, nhops] : prefix2Nhops) {
    auto newRoute = makeRoute(prefix6, nhops);
    if (fib6->getNodeIf(prefix6.str())) {
      fib6->updateNode(prefix6.str(), std::move(newRoute));
    } else {
      fib6->addNode(prefix6.str(), std::move(newRoute));
    }
  }
  newState->publish();
  return consolidate(newState);
}

std::vector<StateDelta> BaseEcmpResourceManagerTest::rmRoutes(
    const std::vector<RoutePrefixV6>& prefix6s) {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  for (const auto& prefix6 : prefix6s) {
    fib6->removeNode(prefix6.str());
  }
  newState->publish();
  return consolidate(newState);
}

std::set<RouteV6::Prefix> BaseEcmpResourceManagerTest::getPrefixesForGroups(
    const EcmpResourceManager::NextHopGroupIds& grpIds) const {
  return facebook::fboss::getPrefixesForGroups(
      *sw_->getEcmpResourceManager(), grpIds);
}

std::set<RouteV6::Prefix>
BaseEcmpResourceManagerTest::getPrefixesWithoutOverrides() const {
  std::set<RouteV6::Prefix> prefixes;
  return getPrefixesForGroups(getGroupsWithoutOverrides());
}

RouteNextHopSet BaseEcmpResourceManagerTest::getNextHops(
    EcmpResourceManager::NextHopGroupId gid) const {
  const auto& nhops2Id = sw_->getEcmpResourceManager()->getNhopsToId();
  for (const auto& nhopsAndId : nhops2Id) {
    if (gid == nhopsAndId.second) {
      return nhopsAndId.first;
    }
  }
  CHECK(false) << " Should never get here";
}

EcmpResourceManager::NextHopGroupIds
BaseEcmpResourceManagerTest::getGroupsWithoutOverrides() const {
  EcmpResourceManager::NextHopGroupIds nonOverrideGids;
  auto grpId2Prefixes = sw_->getEcmpResourceManager()->getGidToPrefixes();
  for (const auto& [_, pfxs] : grpId2Prefixes) {
    auto grpInfo = sw_->getEcmpResourceManager()->getGroupInfo(
        pfxs.begin()->first, pfxs.begin()->second);
    if (!grpInfo->hasOverrides()) {
      nonOverrideGids.insert(grpInfo->getID());
    }
  }
  return nonOverrideGids;
}

EcmpResourceManager::NextHopGroupIds BaseEcmpResourceManagerTest::getAllGroups()
    const {
  EcmpResourceManager::NextHopGroupIds allGroups;
  for (const auto& nhopsAndId : sw_->getEcmpResourceManager()->getNhopsToId()) {
    allGroups.insert(nhopsAndId.second);
  }
  return allGroups;
}

void BaseEcmpResourceManagerTest::assertMergedGroup(
    const EcmpResourceManager::NextHopGroupIds& mergedGroup) const {
  facebook::fboss::assertMergedGroup(
      *sw_->getEcmpResourceManager(), mergedGroup);
}

void BaseEcmpResourceManagerTest::assertGroupsAreUnMerged(
    const EcmpResourceManager::NextHopGroupIds& unmergedGroups) const {
  facebook::fboss::assertGroupsAreUnMerged(
      *sw_->getEcmpResourceManager(), unmergedGroups);
}

TEST_F(BaseEcmpResourceManagerTest, noFibsDelta) {
  auto oldState = state_;
  auto newState = oldState->clone();
  auto ports = newState->getPorts()->modify(&newState);
  auto port1 = ports->getPort("port1")->clone();
  port1->setPortDrainState(cfg::PortDrainState::DRAINED);
  ports->updateNode(port1, hwMatcher());
  auto deltas = consolidate(newState);
  EXPECT_EQ(deltas.size(), 1);
  EXPECT_EQ(deltas.begin()->oldState(), oldState);
  EXPECT_EQ(deltas.begin()->newState(), newState);
  EXPECT_NE(
      deltas.begin()->newState()->getPorts()->getPortIf("port1"), nullptr);
}

TEST_F(BaseEcmpResourceManagerTest, addClassId) {
  auto oldState = state_;
  auto newState = oldState->clone();
  auto fib6 = fib(newState);
  auto newRoute = fib6->cbegin()->second->clone();
  newRoute->updateClassID(cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);
  fib6->updateNode(newRoute);
  auto deltas = consolidate(newState);
  EXPECT_EQ(deltas.size(), 1);
  EXPECT_EQ(deltas.begin()->oldState(), oldState);
  auto postUpdateRoute =
      cfib(deltas.begin()->newState())->getRouteIf(newRoute->prefix());
  EXPECT_NE(postUpdateRoute, nullptr);
  EXPECT_EQ(
      postUpdateRoute->getClassID(), cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);
}

TEST_F(BaseEcmpResourceManagerTest, addCounterId) {
  auto oldState = state_;
  auto newState = oldState->clone();
  auto fib6 = fib(newState);
  auto newRoute = fib6->cbegin()->second->clone();
  const auto& nhopEntry = newRoute->getForwardInfo();
  std::optional<RouteCounterID> counterId("42");
  RouteNextHopEntry newNhopEntry(
      nhopEntry.getNextHopSet(),
      nhopEntry.getAdminDistance(),
      counterId,
      nhopEntry.getClassID());
  newRoute->setResolved(newNhopEntry);
  fib6->updateNode(newRoute);
  auto deltas = consolidate(newState);
  EXPECT_EQ(deltas.size(), 1);
  EXPECT_EQ(deltas.begin()->oldState(), oldState);
  auto postUpdateRoute =
      cfib(deltas.begin()->newState())->getRouteIf(newRoute->prefix());
  EXPECT_NE(postUpdateRoute, nullptr);
  EXPECT_EQ(postUpdateRoute->getForwardInfo().getCounterID(), counterId);
}

TEST_F(BaseEcmpResourceManagerTest, UpdateFailed) {
  auto oldState = state_;
  auto newState = oldState->clone();
  auto fib6 = fib(newState);

  // Create a next hop set
  auto nhops = makeNextHops(9);

  // Add a route with the next hop set
  auto route = makeRoute(nextPrefix(), nhops);
  fib6->addNode(route);

  failUpdate(newState);

  // After update failure, the next hop set should not exist
  auto nhopId = getNhopId(nhops);
  EXPECT_FALSE(nhopId.has_value());
}

TEST_F(BaseEcmpResourceManagerTest, reloadInvalidConfigs) {
  {
    // Both compression threshold and backup group type set
    auto newCfg = onePortPerIntfConfig(42, getBackupEcmpSwitchingMode());
    EXPECT_THROW(sw_->applyConfig("Invalid config", newCfg), FbossError);
  }
  {
    // Resetting backup ecmp type is not allowed
    auto newCfg =
        onePortPerIntfConfig(getEcmpCompressionThresholdPct(), std::nullopt);
    EXPECT_THROW(sw_->applyConfig("Invalid config", newCfg), FbossError);
  }
}
} // namespace facebook::fboss
