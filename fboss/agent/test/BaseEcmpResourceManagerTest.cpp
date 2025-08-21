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
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

RouteNextHopSet makeNextHops(int n) {
  CHECK_LT(n, 255);
  RouteNextHopSet h;
  for (int i = 0; i < n; i++) {
    std::stringstream ss;
    ss << std::hex << i + 1;
    auto ipStr = folly::sformat("2400:db00:2110:{}::2", i);
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

std::shared_ptr<RouteV6> makeRoute(
    const RouteV6::Prefix& pfx,
    const RouteNextHopSet& nextHops) {
  RouteNextHopEntry nhopEntry(nextHops, kDefaultAdminDistance);
  auto rt = std::make_shared<RouteV6>(
      RouteV6::makeThrift(pfx, ClientID(0), nhopEntry));
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
    cfg.interfaces()[p].ipAddresses()->resize(1);
    cfg.interfaces()[p].ipAddresses()[0] =
        folly::sformat("2400:db00:2110:{}::1/64", p);
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
  }
  XLOG(DBG2) << " Consolidator update done";
  XLOG(DBG2) << " SwSwitch update start";
  updateFlowletSwitchingConfig(state);
  updateRoutes(state);
  XLOG(DBG2) << " SwSwitch update done";
  CHECK(state_->isPublished());
  state_ = sw_->getState();
  EXPECT_EQ(state_->getFibs()->getNode(RouterID(0))->getFibV4()->size(), 1);
  /*
   * Assert that EcmpResourceMgr leaves the ports state untouched
   */
  EXPECT_NE(state_->getPorts()->getPortIf("port1"), nullptr);
  {
    // Assert restoration from current state results in no
    // overflow and the route backup group state matches
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
            origRoute->getForwardInfo().getOverrideEcmpSwitchingMode());
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
  std::map<RouteNextHopSet, uint32_t> primaryEcmpTypeGroups2RefCnt;
  for (const auto& [_, route] :
       std::as_const(*cfib(deltas.begin()->oldState()))) {
    if (!route->isResolved() ||
        route->getForwardInfo().normalizedNextHops().size() <= 1) {
      continue;
    }
    if (!route->getForwardInfo().getOverrideEcmpSwitchingMode().has_value()) {
      auto pitr = primaryEcmpTypeGroups2RefCnt.find(
          route->getForwardInfo().normalizedNextHops());
      if (pitr != primaryEcmpTypeGroups2RefCnt.end()) {
        ++pitr->second;
        XLOG(DBG4) << "Processed route: " << route->str()
                   << " primary ECMP groups count unchanged: "
                   << primaryEcmpTypeGroups2RefCnt.size();
      } else {
        primaryEcmpTypeGroups2RefCnt.insert(
            {route->getForwardInfo().normalizedNextHops(), 1});
        XLOG(DBG4) << "Processed route: " << route->str()
                   << " primary ECMP groups count incremented: "
                   << primaryEcmpTypeGroups2RefCnt.size();
      }
    }
  }
  XLOG(DBG2) << " Primary ECMP groups : "
             << primaryEcmpTypeGroups2RefCnt.size();
  // ECMP groups should not exceed the limit.
  EXPECT_LE(
      primaryEcmpTypeGroups2RefCnt.size(),
      consolidator_->getMaxPrimaryEcmpGroups());
  auto routeDeleted = [&primaryEcmpTypeGroups2RefCnt](const auto& oldRoute) {
    XLOG(DBG2) << " Route deleted: " << oldRoute->str();
    if (!oldRoute->isResolved() ||
        oldRoute->getForwardInfo().normalizedNextHops().size() <= 1) {
      return;
    }
    if (oldRoute->getForwardInfo().getOverrideEcmpSwitchingMode().has_value()) {
      return;
    }
    auto pitr = primaryEcmpTypeGroups2RefCnt.find(
        oldRoute->getForwardInfo().normalizedNextHops());
    ASSERT_NE(pitr, primaryEcmpTypeGroups2RefCnt.end());
    EXPECT_GE(pitr->second, 1);
    --pitr->second;
    if (pitr->second == 0) {
      primaryEcmpTypeGroups2RefCnt.erase(pitr);
      XLOG(DBG2) << " Primary ECMP group count decremented to: "
                 << primaryEcmpTypeGroups2RefCnt.size()
                 << " on pfx: " << oldRoute->str();
    }
  };
  auto routeAdded = [&primaryEcmpTypeGroups2RefCnt,
                     this](const auto& newRoute) {
    XLOG(DBG2) << " Route added: " << newRoute->str();
    if (!newRoute->isResolved() ||
        newRoute->getForwardInfo().normalizedNextHops().size() <= 1) {
      return;
    }
    if (newRoute->getForwardInfo().getOverrideEcmpSwitchingMode().has_value()) {
      return;
    }
    auto pitr = primaryEcmpTypeGroups2RefCnt.find(
        newRoute->getForwardInfo().normalizedNextHops());
    if (pitr != primaryEcmpTypeGroups2RefCnt.end()) {
      ++pitr->second;
    } else {
      bool inserted{false};
      std::tie(pitr, inserted) = primaryEcmpTypeGroups2RefCnt.insert(
          {newRoute->getForwardInfo().normalizedNextHops(), 1});
      EXPECT_TRUE(inserted);
      XLOG(DBG2) << " Primary ECMP group count incremented to: "
                 << primaryEcmpTypeGroups2RefCnt.size()
                 << " on pfx: " << newRoute->str();
    }
    EXPECT_LE(
        primaryEcmpTypeGroups2RefCnt.size(),
        consolidator_->getMaxPrimaryEcmpGroups());
  };

  auto idx = 1;
  for (const auto& delta : deltas) {
    XLOG(DBG2) << " Processing delta #" << idx++;
    forEachChangedRoute<folly::IPAddressV6>(
        delta,
        [=](RouterID /*rid*/, const auto& oldRoute, const auto& newRoute) {
          if (!oldRoute->isResolved() && !newRoute->isResolved()) {
            return;
          }
          if (oldRoute->isResolved() && !newRoute->isResolved()) {
            routeDeleted(oldRoute);
            return;
          }
          if (!oldRoute->isResolved() && newRoute->isResolved()) {
            routeAdded(newRoute);
            return;
          }
          // Both old and new are resolved
          CHECK(oldRoute->isResolved() && newRoute->isResolved());
          if (oldRoute->getForwardInfo() != newRoute->getForwardInfo()) {
            // First process as a add route, since ECMP is make before break
            routeAdded(newRoute);
            routeDeleted(oldRoute);
          }
        },
        [=](RouterID /*rid*/, const auto& newRoute) {
          if (newRoute->isResolved()) {
            routeAdded(newRoute);
          }
        },
        [=](RouterID /*rid*/, const auto& oldRoute) {
          if (oldRoute->isResolved()) {
            routeDeleted(oldRoute);
          }
        });
  }
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
  ASSERT_NE(sw_->getEcmpResourceManager(), nullptr);
  // Taken from mock asic
  auto asic = *sw_->getHwAsicTable()->getL3Asics().begin();
  int asicMaxEcmpGroups, maxPct;
  if (getBackupEcmpSwitchingMode()) {
    asicMaxEcmpGroups = *asic->getMaxDlbEcmpGroups();
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
  auto updater = sw_->getRouteUpdater();
  auto constexpr kClientID(ClientID::BGPD);
  StateDelta delta(sw_->getState(), newState);
  processFibsDeltaInHwSwitchOrder(
      delta,
      [&updater](RouterID rid, const auto& /*oldRoute*/, const auto& newRoute) {
        updater.addRoute(
            rid,
            newRoute->prefix().network(),
            newRoute->prefix().mask(),
            kClientID,
            newRoute->getForwardInfo());
      },
      [&updater](RouterID rid, const auto& newRoute) {
        updater.addRoute(
            rid,
            newRoute->prefix().network(),
            newRoute->prefix().mask(),
            kClientID,
            newRoute->getForwardInfo());
      },
      [&updater](RouterID rid, const auto& oldRoute) {
        IpPrefix pfx;
        pfx.ip() = network::toBinaryAddress(oldRoute->prefix().network());
        pfx.prefixLength() = oldRoute->prefix().mask();
        updater.delRoute(rid, pfx, kClientID);
      });

  updater.program();
  assertRibFibEquivalence();
}

void BaseEcmpResourceManagerTest::assertRibFibEquivalence() const {
  waitForStateUpdates(sw_);
  for (const auto& [_, route] : std::as_const(*cfib(sw_->getState()))) {
    auto ribRoute =
        sw_->getRib()->longestMatch(route->prefix().network(), RouterID(0));
    ASSERT_NE(ribRoute, nullptr);
    // TODO - check why are the pointers different even though the
    // forwarding info matches. This is true with or w/o consolidator
    EXPECT_EQ(ribRoute->getForwardInfo(), route->getForwardInfo());
  }
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
  EXPECT_EQ(state_->getFibs()->getNode(RouterID(0))->getFibV4()->size(), 1);
  std::set<RouteNextHopSet> primaryEcmpGroups, backupEcmpGroups;
  for (auto [_, inRoute] : std::as_const(*cfib(endStatePrefixes))) {
    auto route = cfib(targetState)->exactMatch(inRoute->prefix());
    ASSERT_TRUE(route->isResolved());
    ASSERT_NE(route, nullptr);
    auto consolidatorGrpInfo = consolidatorToCheck->getGroupInfo(
        RouterID(0), inRoute->prefix().toCidrNetwork());
    bool isEcmpRoute = route->isResolved() &&
        route->getForwardInfo().getNextHopSet().size() > 1;
    if (isEcmpRoute) {
      ASSERT_NE(consolidatorGrpInfo, nullptr);
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
        EXPECT_EQ(
            std::tie(swGroupId, swRouteUsageCount, swIsBackupEcmpType),
            std::tie(
                consolidatorGroupId,
                consolidatorRouteUsageCount,
                consolidatorIsBackupEcmpType));
      }
    }
    if (overflowPrefixes.find(route->prefix()) != overflowPrefixes.end()) {
      EXPECT_TRUE(route->getForwardInfo().hasOverrideSwitchingModeOrNhops())
          << " expected route " << route->str()
          << " to have override ECMP group type or ecmp nhops";
      if (getBackupEcmpSwitchingMode()) {
        EXPECT_EQ(
            route->getForwardInfo().getOverrideEcmpSwitchingMode(),
            consolidatorToCheck->getBackupEcmpSwitchingMode());
        EXPECT_TRUE(consolidatorGrpInfo->isBackupEcmpGroupType());
        if (isEcmpRoute) {
          backupEcmpGroups.insert(route->getForwardInfo().normalizedNextHops());
        }
      }
      if (getEcmpCompressionThresholdPct()) {
        EXPECT_TRUE(route->getForwardInfo().getOverrideNextHops().has_value());
        EXPECT_EQ(
            route->getForwardInfo().getOverrideNextHops(),
            consolidatorToCheck
                ->getGroupInfo(RouterID(0), route->prefix().toCidrNetwork())
                ->getOverrideNextHops());
        // Merged groups also take up primary ecmp groups
        primaryEcmpGroups.insert(route->getForwardInfo().normalizedNextHops());
      }
    } else {
      EXPECT_FALSE(route->getForwardInfo().hasOverrideSwitchingModeOrNhops());
      if (isEcmpRoute) {
        EXPECT_FALSE(consolidatorGrpInfo->isBackupEcmpGroupType());
        EXPECT_FALSE(consolidatorGrpInfo->hasOverrideNextHops());
        primaryEcmpGroups.insert(route->getForwardInfo().normalizedNextHops());
      }
    }
  }
  if (checkStats) {
    EXPECT_EQ(
        sw_->stats()->getPrimaryEcmpGroupsExhausted(),
        backupEcmpGroups.size() ? 1 : 0);
    EXPECT_EQ(
        sw_->stats()->getPrimaryEcmpGroupsCount(), primaryEcmpGroups.size());
    EXPECT_EQ(
        sw_->stats()->getBackupEcmpGroupsCount(), backupEcmpGroups.size());
  }
}

std::vector<StateDelta> BaseEcmpResourceManagerTest::addOrUpdateRoute(
    const RoutePrefixV6& prefix6,
    const RouteNextHopSet& nhops) {
  auto newRoute = makeRoute(prefix6, nhops);
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  if (fib6->getNodeIf(prefix6.str())) {
    fib6->updateNode(prefix6.str(), std::move(newRoute));
  } else {
    fib6->addNode(prefix6.str(), std::move(newRoute));
  }
  newState->publish();
  return consolidate(newState);
}

std::vector<StateDelta> BaseEcmpResourceManagerTest::rmRoute(
    const RoutePrefixV6& prefix6) {
  auto newState = state_->clone();
  auto fib6 = fib(newState);
  fib6->removeNode(prefix6.str());
  newState->publish();
  return consolidate(newState);
}

std::set<RouteV6::Prefix> BaseEcmpResourceManagerTest::getPrefixesForGroups(
    const EcmpResourceManager::NextHopGroupIds& grpIds) const {
  auto grpId2Prefixes = sw_->getEcmpResourceManager()->getGroupIdToPrefix();
  std::set<RouteV6::Prefix> prefixes;
  for (auto grpId : grpIds) {
    for (const auto& [_, pfx] : grpId2Prefixes.at(grpId)) {
      prefixes.insert(RouteV6::Prefix(pfx.first.asV6(), pfx.second));
    }
  }
  return prefixes;
}

std::set<RouteV6::Prefix>
BaseEcmpResourceManagerTest::getPrefixesWithoutOverrides() const {
  std::set<RouteV6::Prefix> prefixes;
  EcmpResourceManager::NextHopGroupIds nonOverrideGids;
  auto grpId2Prefixes = sw_->getEcmpResourceManager()->getGroupIdToPrefix();
  for (const auto& [_, pfxs] : grpId2Prefixes) {
    auto grpInfo = sw_->getEcmpResourceManager()->getGroupInfo(
        pfxs.begin()->first, pfxs.begin()->second);
    if (!grpInfo->hasOverrides()) {
      nonOverrideGids.insert(grpInfo->getID());
    }
  }
  return getPrefixesForGroups(nonOverrideGids);
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
