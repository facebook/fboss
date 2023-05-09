/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/logging/xlog.h>
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

extern "C" {
#include <bcm/field.h>
#include <bcm/types.h>
}

using namespace facebook::fboss;
using namespace facebook::fboss::utility;
using facebook::fboss::utility::kBaseVlanId;
using folly::CIDRNetwork;
using folly::IPAddress;

namespace {
constexpr auto kAclName = "acl0";
const RouterID kRid(0);
HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}
} // namespace

namespace facebook::fboss {

class BcmAclNexthopTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports(5);
    for (int i = 0; i < ports.size(); ++i) {
      ports[i] = masterLogicalPortIds()[i];
    }
    return utility::onePortPerInterfaceConfig(getHwSwitch(), ports);
  }

  cfg::SwitchConfig testConfig(std::string vipIp, std::string nexthopIp) const {
    auto cfg = initialConfig();
    cfg::AclEntry acl;
    auto numCfgAcls = cfg.acls()->size();
    cfg.acls()->resize(numCfgAcls + 1);
    cfg.acls()[numCfgAcls].name() = kAclName;
    cfg.acls()[numCfgAcls].dstIp() = vipIp;
    cfg.acls()[numCfgAcls].vlanID() = 2001;

    auto redirect = cfg::RedirectToNextHopAction();
    cfg::RedirectNextHop nhop;
    nhop.ip() = nexthopIp;
    redirect.redirectNextHops()->push_back(nhop);
    auto action = cfg::MatchAction();
    action.redirectToNextHop() = redirect;
    utility::addMatcher(&cfg, kAclName, action);
    return cfg;
  }

  std::shared_ptr<BcmMultiPathNextHop> getBcmMultiPathNextHop(
      const RouteNextHopSet& nexthops) const {
    XLOG(DBG3) << "routerId: " << kRid << ", nexthops: " << nexthops;
    return getHwSwitch()
        ->writableMultiPathNextHopTable()
        ->referenceOrEmplaceNextHop(BcmMultiPathNextHopKey(kRid, nexthops));
  }

  void updateAcl(std::string name, RouteNextHopSet nexthops) {
    auto newState = getProgrammedState()->clone();
    auto origAclEntry = newState->getMultiSwitchAcls()->getNodeIf(name);
    auto newAclEntry = origAclEntry->modify(&newState, scope());
    // THRIFT_COPY
    MatchAction action =
        MatchAction::fromThrift(newAclEntry->getAclAction()->toThrift());
    const auto& redirect = action.getRedirectToNextHop();
    action.setRedirectToNextHop(
        std::make_pair(redirect.value().first, nexthops));
    newAclEntry->setAclAction(action);
    newAclEntry->setEnabled(nexthops.size() ? true : false);
    applyNewState(newState);
  }

  void verifyAclEntryProgramming(
      const std::shared_ptr<AclEntry>& swAcl,
      const RouteNextHopSet& nexthops) {
    auto bcmSwitch = static_cast<BcmSwitch*>(this->getHwSwitch());
    bcmSwitch->getAclTable()->forFilteredEach(
        [=](const auto& pair) {
          const auto& bcmAclEntry = pair.second;
          return bcmAclEntry->getID() == swAcl->getID();
        },
        [&](const auto& pair) {
          const auto& bcmAclEntry = pair.second;
          auto bcmAclHandle = bcmAclEntry->getHandle();
          uint32_t egressId;
          uint32_t param1;
          bcm_field_action_get(
              0, bcmAclHandle, bcmFieldActionL3Switch, &egressId, &param1);
          bcm_if_t expectedEgressId;
          if (nexthops.size() > 0) {
            std::shared_ptr<BcmMultiPathNextHop> bcmNexthop =
                getBcmMultiPathNextHop(nexthops);
            expectedEgressId = bcmNexthop->getEgressId();
          } else {
            expectedEgressId = getHwSwitch()->getDropEgressId();
          }
          EXPECT_EQ(egressId, static_cast<uint32_t>(expectedEgressId));
          EXPECT_TRUE(BcmAclEntry::isStateSame(
              bcmSwitch,
              bcmSwitch->getPlatform()->getAsic()->getDefaultACLGroupID(),
              bcmAclHandle,
              swAcl));
        });
  }
};

TEST_F(BcmAclNexthopTest, AddAclWithRedirect) {
  int intf1 = kBaseVlanId; /* config has one interface */
  std::vector<IPAddress> nexthopIps = {
      // These are the recursively resolved nexthops
      // to reach VIP nexthops
      IPAddress("1.1.1.10"),
      IPAddress("1.1.2.10"),
      IPAddress("1.1.3.10"),
  };
  RouteNextHopSet nexthops;
  int idx = 0;
  for (auto nhIp : nexthopIps) {
    LabelForwardingAction labelAction(
        LabelForwardingAction::LabelForwardingType::PUSH,
        LabelForwardingAction::LabelStack{2001 + idx});
    nexthops.insert(ResolvedNextHop(
        nhIp, InterfaceID(intf1), UCMP_DEFAULT_WEIGHT, labelAction));
    ++idx;
  }
  auto setup = [=]() {
    std::string vipIp = "10.10.10.1";
    std::string vipNexthopIp = "123.0.1.1";
    applyNewConfig(testConfig(vipIp, vipNexthopIp));
    updateAcl(kAclName, nexthops);
  };

  auto verify = [=]() {
    const auto& acl =
        getProgrammedState()->getMultiSwitchAcls()->getNodeIf(kAclName);
    verifyAclEntryProgramming(acl, nexthops);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclNexthopTest, NoResolvedNexthops) {
  RouteNextHopSet nexthops;
  auto setup = [=]() {
    std::string vipIp = "10.10.10.1";
    std::string vipNexthopIp = "123.0.1.1";
    applyNewConfig(testConfig(vipIp, vipNexthopIp));
    updateAcl(kAclName, nexthops);
  };

  auto verify = [=]() {
    const auto& acl =
        getProgrammedState()->getMultiSwitchAcls()->getNodeIf(kAclName);
    verifyAclEntryProgramming(acl, nexthops);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclNexthopTest, AddAclWithRedirectV6) {
  int intf1 = kBaseVlanId;
  std::vector<IPAddress> nexthopIps = {
      // These are the recursively resolved nexthops
      // to reach VIP nexthops
      IPAddress("1:0::10"),
      IPAddress("1:1::10"),
      IPAddress("1:2::10"),
  };
  RouteNextHopSet nexthops;
  int idx = 0;
  for (auto nhIp : nexthopIps) {
    LabelForwardingAction labelAction(
        LabelForwardingAction::LabelForwardingType::PUSH,
        LabelForwardingAction::LabelStack{2001 + idx});
    nexthops.insert(ResolvedNextHop(
        nhIp, InterfaceID(intf1), UCMP_DEFAULT_WEIGHT, labelAction));
    ++idx;
  }
  auto setup = [=]() {
    std::string vipIp = "1010:1010::1";
    std::string vipNexthopIp = "1001::1";
    applyNewConfig(testConfig(vipIp, vipNexthopIp));
    updateAcl(kAclName, nexthops);
  };
  auto verify = [=]() {
    const auto& acl =
        getProgrammedState()->getMultiSwitchAcls()->getNodeIf(kAclName);
    verifyAclEntryProgramming(acl, nexthops);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclNexthopTest, NoResolvedNexthopsV6) {
  RouteNextHopSet nexthops;
  auto setup = [=]() {
    std::string vipIp = "1010:1010::1";
    std::string vipNexthopIp = "1001::1";
    applyNewConfig(testConfig(vipIp, vipNexthopIp));
    updateAcl(kAclName, nexthops);
  };

  auto verify = [=]() {
    const auto& acl =
        getProgrammedState()->getMultiSwitchAcls()->getNodeIf(kAclName);
    verifyAclEntryProgramming(acl, nexthops);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
