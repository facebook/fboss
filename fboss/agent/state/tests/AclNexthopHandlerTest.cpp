/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/AclMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include "fboss/agent/GtestDefs.h"

using namespace facebook::fboss;
using facebook::fboss::InterfaceID;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

namespace facebook::fboss {

namespace {
constexpr AdminDistance DISTANCE = AdminDistance::STATIC_ROUTE;

HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}

}; // namespace

template <typename AddrType>
class AclNexthopHandlerTest : public ::testing::Test {
 public:
  using Func = folly::Function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;
  using AddrT = AddrType;

  cfg::SwitchConfig testConfig() {
    auto cfg = testConfigA();

    int prevVlanSize = cfg.vlans()->size();
    cfg.vlans()->resize(prevVlanSize + 4);
    int startVid = 100;
    for (int vid = startVid, idx = prevVlanSize; vid < startVid + 4;
         ++vid, ++idx) {
      cfg.vlans()[idx].id() = vid;
      cfg.vlans()[idx].name() = fmt::format("Vlan{}", vid);
      cfg.vlans()[idx].intfID() = vid;
    }

    cfg.interfaces()->resize(prevVlanSize + 4);
    for (int vid = startVid, idx = prevVlanSize; vid < startVid + 4;
         ++vid, ++idx) {
      cfg.interfaces()[idx].intfID() = vid;
      cfg.interfaces()[idx].routerID() = 0;
      cfg.interfaces()[idx].vlanID() = vid;
      cfg.interfaces()[idx].name() = fmt::format("interface{}", vid);
      cfg.interfaces()[idx].mac() = fmt::format("00:02:00:00:00:{:02x}", vid);
      cfg.interfaces()[idx].mtu() = 9000;
      cfg.interfaces()[idx].ipAddresses()->resize(4);
      cfg.interfaces()[idx].ipAddresses()[0] =
          fmt::format("100.0.{}.1/24", vid);
      cfg.interfaces()[idx].ipAddresses()[1] =
          fmt::format("172.16.{}.1/24", vid);
      cfg.interfaces()[idx].ipAddresses()[2] =
          fmt::format("1000:{:04x}::0001/64", vid);
      cfg.interfaces()[idx].ipAddresses()[3] = "fe80::/64"; // link local
    }
    return cfg;
  }

  void SetUp() override {
    auto config = testConfig();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
  }

  void verifyStateUpdate(Func func) {
    runInUpdateEventBaseAndWait(std::move(func));
  }

  void TearDown() override {
    schedulePendingTestStateUpdates();
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, func);
  }

  std::vector<std::string> getAclNexthopIps(int numNextHops) const {
    std::vector<std::string> nexthopIps(numNextHops);
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      for (int i = 0; i < numNextHops; ++i) {
        nexthopIps[i] = fmt::format("123.0.{}.1", i);
      }
    } else {
      for (int i = 0; i < numNextHops; ++i) {
        nexthopIps[i] = fmt::format("1234:{}::1", i);
      }
    }
    return nexthopIps;
  }

  RoutePrefix<AddrT> makePrefix(std::string prefixStr) {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return makePrefixV4(prefixStr);
    } else {
      return makePrefixV6(prefixStr);
    }
  }

  std::vector<std::pair<InterfaceID, std::string>> getResolvedNexthops(
      int numNextHops) const {
    std::vector<std::pair<InterfaceID, std::string>> intfAndIps(numNextHops);
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      for (int i = 0, vid = 100; i < numNextHops; ++i, ++vid) {
        intfAndIps[i] = {InterfaceID(vid), fmt::format("100.0.{}.1", vid)};
      }
    } else {
      for (int i = 0, vid = 100; i < numNextHops; ++i, ++vid) {
        intfAndIps[i] = {
            InterfaceID(vid), fmt::format("1000:{:04x}::0001", vid)};
      }
    }
    return intfAndIps;
  }

  std::vector<int32_t> getOutInterfaces(int numInterfaces) const {
    std::vector<int32_t> intfIDs;
    for (int i = 0, vid = 100; i < numInterfaces; ++i, ++vid) {
      intfIDs.emplace_back(vid);
    }
    return intfIDs;
  }

  std::vector<std::string> getMatchingPrefixes(int numPrefixes) const {
    std::vector<std::string> prefixes(numPrefixes);
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      for (int i = 0; i < numPrefixes; ++i) {
        prefixes[i] = fmt::format("123.0.{}.0/24", i);
      }
    } else {
      for (int i = 0; i < numPrefixes; ++i) {
        prefixes[i] = fmt::format("1234:{}::0/64", i);
      }
    }
    return prefixes;
  }

  void addRoute(const RoutePrefix<AddrT>& prefix, RouteNextHopSet nexthops) {
    auto updater = sw_->getRouteUpdater();
    updater.addRoute(
        RouterID(0),
        prefix.network(),
        prefix.mask(),
        ClientID(1000),
        RouteNextHopEntry(nexthops, DISTANCE));
    updater.program();
  }

  void delRoute(const RoutePrefix<AddrT>& prefix) {
    auto updater = sw_->getRouteUpdater();
    updater.delRoute(
        RouterID(0), prefix.network(), prefix.mask(), ClientID(1000));
    updater.program();
  }

  std::shared_ptr<SwitchState> addAcl(
      const std::shared_ptr<SwitchState>& state,
      const std::string& name,
      const std::vector<std::string>& nexthopIps,
      const std::vector<int32_t>& intfIDs = {}) {
    auto aclEntry = std::make_shared<AclEntry>(0, name);
    auto cfgRedirectToNextHop = cfg::RedirectToNextHopAction();
    auto idx = 0;
    for (auto nhIp : nexthopIps) {
      cfg::RedirectNextHop nhop;
      nhop.ip_ref() = nhIp;
      if (intfIDs.size()) {
        nhop.intfID_ref() = intfIDs[idx];
      }
      cfgRedirectToNextHop.redirectNextHops()->push_back(nhop);
      idx++;
    }
    auto redirectToNextHop = MatchAction::RedirectToNextHopAction();
    redirectToNextHop.first = cfgRedirectToNextHop;
    MatchAction action = MatchAction();
    action.setRedirectToNextHop(redirectToNextHop);
    aclEntry->setAclAction(action);
    auto newState = state->isPublished() ? state->clone() : state;
    auto MultiSwitchAclMap = newState->getMultiSwitchAcls()->modify(&newState);
    MultiSwitchAclMap->addNode(aclEntry, scope());
    return newState;
  }

  RouteNextHopSet makeResolvedMplsNextHops(
      std::vector<std::pair<InterfaceID, std::string>> intfAndIPs,
      int baseLabel) {
    RouteNextHopSet nhops;
    int idx = 0;
    for (auto intfAndIP : intfAndIPs) {
      LabelForwardingAction labelAction(
          LabelForwardingAction::LabelForwardingType::PUSH,
          LabelForwardingAction::LabelStack{baseLabel + idx});
      nhops.emplace(ResolvedNextHop(
          IPAddress(intfAndIP.second),
          intfAndIP.first,
          UCMP_DEFAULT_WEIGHT,
          labelAction));
      ++idx;
    }
    return nhops;
  }

 protected:
  void verifyResolvedNexthopsInAclAction(
      const std::string& aclName,
      const RouteNextHopSet& expectedNexthops) {
    auto verifyResolvedNexthops = [&]() {
      auto state = sw_->getState();
      auto aclEntry = state->getMultiSwitchAcls()->getNodeIf(aclName);
      EXPECT_NE(aclEntry, nullptr);
      const auto& action = aclEntry->getAclAction();
      auto matchAction = MatchAction::fromThrift(action->toThrift());
      auto resolvedNexthopSet =
          matchAction.getRedirectToNextHop().value().second;
      XLOG(DBG3) << "expected nexthops: " << expectedNexthops
                 << ", resolved nexthops: " << resolvedNexthopSet;
      // Since the route is deleted, expect no resolved nexthops
      EXPECT_EQ(resolvedNexthopSet, expectedNexthops);
      EXPECT_EQ(*aclEntry->isEnabled(), expectedNexthops.size() ? true : false);
    };
    verifyStateUpdate(verifyResolvedNexthops);
  }

  void runInUpdateEventBaseAndWait(Func func) {
    auto* evb = sw_->getUpdateEvb();
    evb->runInEventBaseThreadAndWait(std::move(func));
  }

  void schedulePendingTestStateUpdates() {
    runInUpdateEventBaseAndWait([]() {});
  }

  std::unique_ptr<HwTestHandle> handle_;
  std::unique_ptr<ThriftHandler> thriftHandler{nullptr};
  SwSwitch* sw_;
};

using AclNexthopHandlerTestTypes =
    ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

const std::string kAclName = "acl0";
const RouteNextHopSet kEmptyNexthopSet;

TYPED_TEST_CASE(AclNexthopHandlerTest, AclNexthopHandlerTestTypes);

TYPED_TEST(AclNexthopHandlerTest, UnresolvedAclNextHop) {
  this->updateState(
      "UnresolvedAclNextHop", [=](const std::shared_ptr<SwitchState>& state) {
        return this->addAcl(state, kAclName, this->getAclNexthopIps(1));
      });

  this->verifyStateUpdate([=]() {
    auto state = this->sw_->getState();
    auto aclEntry = state->getAcls()->getEntry(kAclName);
    EXPECT_NE(aclEntry, nullptr);
    auto action = aclEntry->getAclAction();
    auto resolvedNexthopSet = util::toRouteNextHopSet(
        action->template cref<switch_state_tags::redirectToNextHop>()
            ->template cref<switch_state_tags::resolvedNexthops>()
            ->toThrift());
    EXPECT_EQ(resolvedNexthopSet.size(), 0);
    EXPECT_EQ(*aclEntry->isEnabled(), false);
  });
}

TYPED_TEST(AclNexthopHandlerTest, ResolvedAclNextHopSingleNexthop) {
  this->updateState(
      "UnresolvedAclNextHop", [=](const std::shared_ptr<SwitchState>& state) {
        return this->addAcl(state, kAclName, this->getAclNexthopIps(1));
      });

  auto matchingPrefixes = this->getMatchingPrefixes(1);
  auto nexthopIps = this->getResolvedNexthops(2);
  auto longestPrefix1 = this->makePrefix(matchingPrefixes[0]);
  RouteNextHopSet nexthops1 =
      makeResolvedNextHops(nexthopIps, UCMP_DEFAULT_WEIGHT);
  this->addRoute(longestPrefix1, nexthops1);
  this->verifyResolvedNexthopsInAclAction(kAclName, nexthops1);

  // Now change the route nexthops and verify the
  // change is reflected on the ACL redirect action
  this->delRoute(longestPrefix1);
  this->verifyResolvedNexthopsInAclAction(kAclName, kEmptyNexthopSet);

  RouteNextHopSet nexthops2 = makeResolvedNextHops(
      {
          nexthopIps[0],
      },
      UCMP_DEFAULT_WEIGHT);
  this->addRoute(longestPrefix1, nexthops2);
  this->verifyResolvedNexthopsInAclAction(kAclName, nexthops2);
}

TYPED_TEST(AclNexthopHandlerTest, ResolvedAclNextHopMultiNexthop) {
  this->updateState(
      "UnresolvedAclNextHop", [=](const std::shared_ptr<SwitchState>& state) {
        return this->addAcl(state, kAclName, this->getAclNexthopIps(2));
      });

  auto nexthopIps = this->getResolvedNexthops(4);
  RouteNextHopSet nexthops1 = makeResolvedNextHops(
      {
          nexthopIps[0],
          nexthopIps[1],
      },
      UCMP_DEFAULT_WEIGHT);
  RouteNextHopSet nexthops2 = makeResolvedNextHops(
      {
          nexthopIps[2],
          nexthopIps[3],
      },
      UCMP_DEFAULT_WEIGHT);
  auto matchingPrefixes = this->getMatchingPrefixes(2);
  auto longestPrefix1 = this->makePrefix(matchingPrefixes[0]);
  this->addRoute(longestPrefix1, nexthops1);
  auto longestPrefix2 = this->makePrefix(matchingPrefixes[1]);
  this->addRoute(longestPrefix2, nexthops2);

  RouteNextHopSet expectedNexthops;
  expectedNexthops.merge(nexthops1);
  expectedNexthops.merge(nexthops2);
  this->verifyResolvedNexthopsInAclAction(kAclName, expectedNexthops);

  // Now change the route nexthops and verify the
  // change is reflected on the ACL redirect action
  this->delRoute(longestPrefix1);
  this->delRoute(longestPrefix2);
  this->verifyResolvedNexthopsInAclAction(kAclName, kEmptyNexthopSet);

  RouteNextHopSet nexthops3 = makeResolvedNextHops(
      {
          nexthopIps[0],
      },
      UCMP_DEFAULT_WEIGHT);
  RouteNextHopSet nexthops4 = makeResolvedNextHops(
      {
          nexthopIps[2],
      },
      UCMP_DEFAULT_WEIGHT);
  this->addRoute(longestPrefix1, nexthops3);
  this->addRoute(longestPrefix2, nexthops4);

  expectedNexthops.clear();
  expectedNexthops.merge(nexthops3);
  expectedNexthops.merge(nexthops4);
  this->verifyResolvedNexthopsInAclAction(kAclName, expectedNexthops);
}

// Test with nexthops that include MPLS action
TYPED_TEST(AclNexthopHandlerTest, MplsNexthops) {
  this->updateState(
      "UnresolvedAclNextHop", [=](const std::shared_ptr<SwitchState>& state) {
        return this->addAcl(state, kAclName, this->getAclNexthopIps(2));
      });

  auto nexthopIps = this->getResolvedNexthops(4);
  RouteNextHopSet nexthops1 = this->makeResolvedMplsNextHops(
      {
          nexthopIps[0],
          nexthopIps[1],
      },
      2000);
  RouteNextHopSet nexthops2 = this->makeResolvedMplsNextHops(
      {
          nexthopIps[2],
          nexthopIps[3],
      },
      3000);
  auto matchingPrefixes = this->getMatchingPrefixes(2);
  auto longestPrefix1 = this->makePrefix(matchingPrefixes[0]);
  this->addRoute(longestPrefix1, nexthops1);
  auto longestPrefix2 = this->makePrefix(matchingPrefixes[1]);
  this->addRoute(longestPrefix2, nexthops2);

  RouteNextHopSet expectedNexthops;
  expectedNexthops.merge(nexthops1);
  expectedNexthops.merge(nexthops2);
  this->verifyResolvedNexthopsInAclAction(kAclName, expectedNexthops);

  // Now change the route nexthops and verify the
  // change is reflected on the ACL redirect action
  this->delRoute(longestPrefix1);
  this->delRoute(longestPrefix2);
  this->verifyResolvedNexthopsInAclAction(kAclName, kEmptyNexthopSet);

  RouteNextHopSet nexthops3 = this->makeResolvedMplsNextHops(
      {
          nexthopIps[0],
      },
      2000);
  RouteNextHopSet nexthops4 = this->makeResolvedMplsNextHops(
      {
          nexthopIps[2],
      },
      3000);
  this->addRoute(longestPrefix1, nexthops3);
  this->addRoute(longestPrefix2, nexthops4);

  expectedNexthops.clear();
  expectedNexthops.merge(nexthops3);
  expectedNexthops.merge(nexthops4);
  this->verifyResolvedNexthopsInAclAction(kAclName, expectedNexthops);
}

// Verify acl nexthop resolution when outgoing interfaces are specified.
// The configuration specifies an out interface in addition to a recursive
// nexthop address and ACL nexthop resolution logic should select a nexthop
// for redirection only if the interface matches.
TYPED_TEST(AclNexthopHandlerTest, ResolvedAclNextHopMultiNexthopWithInterface) {
  auto outIntfs = this->getOutInterfaces(4);
  // Create acl with one outinterface from each nexthop
  this->updateState(
      "UnresolvedAclNextHop", [=](const std::shared_ptr<SwitchState>& state) {
        return this->addAcl(
            state,
            kAclName,
            this->getAclNexthopIps(2),
            {outIntfs[0], outIntfs[2]});
      });

  auto nexthopIps = this->getResolvedNexthops(4);
  RouteNextHopSet nexthops1 = makeResolvedNextHops(
      {
          nexthopIps[0],
          nexthopIps[1],
      },
      UCMP_DEFAULT_WEIGHT);
  RouteNextHopSet nexthops2 = makeResolvedNextHops(
      {
          nexthopIps[2],
          nexthopIps[3],
      },
      UCMP_DEFAULT_WEIGHT);
  auto matchingPrefixes = this->getMatchingPrefixes(2);
  auto longestPrefix1 = this->makePrefix(matchingPrefixes[0]);
  this->addRoute(longestPrefix1, nexthops1);
  auto longestPrefix2 = this->makePrefix(matchingPrefixes[1]);
  this->addRoute(longestPrefix2, nexthops2);

  RouteNextHopSet expectedNexthops;
  RouteNextHopSet expected = makeResolvedNextHops(
      {
          nexthopIps[0],
          nexthopIps[2],
      },
      UCMP_DEFAULT_WEIGHT);
  expectedNexthops.merge(expected);
  this->verifyResolvedNexthopsInAclAction(kAclName, expectedNexthops);

  // Now change the route nexthops and verify the
  // change is reflected on the ACL redirect action
  this->delRoute(longestPrefix1);
  this->delRoute(longestPrefix2);
  this->verifyResolvedNexthopsInAclAction(kAclName, kEmptyNexthopSet);

  // Change to nexthops with no matching interface ids
  RouteNextHopSet nexthops3 = makeResolvedNextHops(
      {
          nexthopIps[1],
      },
      UCMP_DEFAULT_WEIGHT);
  RouteNextHopSet nexthops4 = makeResolvedNextHops(
      {
          nexthopIps[3],
      },
      UCMP_DEFAULT_WEIGHT);
  this->addRoute(longestPrefix1, nexthops3);
  this->addRoute(longestPrefix2, nexthops4);

  this->verifyResolvedNexthopsInAclAction(kAclName, kEmptyNexthopSet);

  this->delRoute(longestPrefix1);
  this->delRoute(longestPrefix2);

  // change back to matching interface ids
  RouteNextHopSet nexthops5 = makeResolvedNextHops(
      {
          nexthopIps[0],
      },
      UCMP_DEFAULT_WEIGHT);
  RouteNextHopSet nexthops6 = makeResolvedNextHops(
      {
          nexthopIps[2],
      },
      UCMP_DEFAULT_WEIGHT);
  this->addRoute(longestPrefix1, nexthops5);
  this->addRoute(longestPrefix2, nexthops6);

  this->verifyResolvedNexthopsInAclAction(kAclName, expectedNexthops);
}
} // namespace facebook::fboss
