/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmTestRouteUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/state/Interface.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

extern "C" {
#include <bcm/error.h>
#include <bcm/l3.h>
}

using folly::CIDRNetwork;
using folly::IPAddress;
using namespace facebook::fboss;
using namespace facebook::fboss::utility;
typedef std::pair<bcm_vrf_t, folly::IPAddress> VrfAndIP;
typedef boost::container::flat_map<VrfAndIP, bcm_l3_host_t> VrfAndIP2Host;

namespace {
void initL3HostWithAddr(bcm_l3_host_t* l3Host, const folly::IPAddress& addr) {
  bcm_l3_host_t_init(l3Host);
  if (addr.isV4()) {
    l3Host->l3a_ip_addr = addr.asV4().toLongHBO();
  } else {
    memcpy(
        &l3Host->l3a_ip6_addr,
        addr.asV6().toByteArray().data(),
        sizeof(l3Host->l3a_ip6_addr));
    l3Host->l3a_flags |= BCM_L3_IP6;
  }
}

int hostTraversalCallback(
    int /*unit*/,
    int /*index*/,
    bcm_l3_host_t* host,
    void* userData) {
  VrfAndIP2Host* l3HostMap = static_cast<VrfAndIP2Host*>(userData);
  auto ip = host->l3a_flags & BCM_L3_IP6
      ? folly::IPAddress::fromBinary(
            folly::ByteRange(host->l3a_ip6_addr, sizeof(host->l3a_ip6_addr)))
      : folly::IPAddress::fromLongHBO(host->l3a_ip_addr);
  l3HostMap->emplace(std::make_pair(host->l3a_vrf, ip), *host);
  return 0;
}

VrfAndIP2Host getL3HostFromHw(int unit) {
  VrfAndIP2Host l3HostMap;

  bcm_l3_info_t l3Info;
  bcm_l3_info_t_init(&l3Info);
  bcm_l3_info(unit, &l3Info);
  // Traverse V4 hosts
  bcm_l3_host_traverse(
      unit, 0, 0, l3Info.l3info_max_host, hostTraversalCallback, &l3HostMap);
  // Traverse V6 hosts
  bcm_l3_host_traverse(
      unit,
      BCM_L3_IP6,
      0,
      // Diag shell uses this for getting # of v6 host entries
      l3Info.l3info_max_host / 2,
      hostTraversalCallback,
      &l3HostMap);

  return l3HostMap;
}
} // unnamed namespace

namespace facebook::fboss {

class BcmHostTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }

  void checkBcmHostMatchInHw(const BcmHost* host, bool isLocalIp) {
    // check host exists in asic
    bcm_l3_host_t l3Host;
    initL3HostWithAddr(&l3Host, host->getHostKey().addr());
    auto rc = bcm_l3_host_find(getUnit(), &l3Host);
    bcmCheckError(
        rc, "failed to find L3 host object for ", host->getHostKey().str());
    CHECK_EQ(host->getHostKey().getVrf(), l3Host.l3a_vrf);
    CHECK_EQ(host->getEgressId(), l3Host.l3a_intf);
    if (isLocalIp) {
      if (host->getHostKey().addr().isV4()) {
        CHECK_EQ(host->getLookupClassId(), BcmAclEntry::kLocalIp4DstClassL3Id);
      } else {
        CHECK_EQ(host->getLookupClassId(), BcmAclEntry::kLocalIp6DstClassL3Id);
      }
    } else {
      // other non local ip host should be 0
      CHECK_EQ(host->getLookupClassId(), 0);
    }
    CHECK_EQ(
        host->getLookupClassId(), BcmHost::getLookupClassFromL3Host(l3Host));
  }

  void checkSwHwBcmHostNum(int expectedNum) {
    // we also assign link local address to each interface by default, but
    // we don't program it to HW
    CHECK_EQ(getHwSwitch()->getHostTable()->getNumBcmHost(), expectedNum + 1);
    CHECK_EQ(getL3HostFromHw(getUnit()).size(), expectedNum);
  }

  BcmHost* getExistingBcmHost(InterfaceID intfID, folly::IPAddress ipAddress) {
    const BcmIntf* intf = getHwSwitch()->getIntfTable()->getBcmIntf(intfID);
    bcm_vrf_t vrf = BcmSwitch::getBcmVrfId(intf->getInterface()->getRouterID());
    return getHwSwitch()->getHostTable()->getBcmHost(
        BcmHostKey(vrf, ipAddress, intfID));
  }
};

TEST_F(BcmHostTest, CreateV4AndV6L3Host) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    InterfaceID intfID = InterfaceID(utility::kBaseVlanId);
    const auto& swIntf =
        getProgrammedState()->getInterfaces()->getInterface(intfID);
    int expectedHostNum{0};
    for (const auto& ipAddressPair : swIntf->getAddresses()) {
      if (!ipAddressPair.first.isLinkLocal()) {
        expectedHostNum++;
        checkBcmHostMatchInHw(
            getExistingBcmHost(swIntf->getID(), ipAddressPair.first), true);
      }
    }
    checkSwHwBcmHostNum(expectedHostNum);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmHostTest, CreateInterfaceWithoutIp) {
  auto setup = [=]() {
    applyNewConfig(utility::oneL3IntfNoIPAddrConfig(
        getHwSwitch(), masterLogicalPortIds()[0]));
  };
  auto verify = [=]() { checkSwHwBcmHostNum(0); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmHostTest, DeleteV4AndV6L3Host) {
  auto setup = [=]() {
    // first apply config which interface has ip addresses assigned
    applyNewConfig(initialConfig());
    // check HW side should has only 2 l3_host created
    CHECK_EQ(getL3HostFromHw(getUnit()).size(), 2);
    // then we re-apply config with 0 ip address assigned interface
    applyNewConfig(utility::oneL3IntfNoIPAddrConfig(
        getHwSwitch(), masterLogicalPortIds()[0]));
  };
  auto verify = [=]() { checkSwHwBcmHostNum(0); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmHostTest, HostRouteLookupClassNotSet) {
  if (!getPlatform()->canUseHostTableForHostRoutes()) {
    return;
  }
  std::array<std::pair<CIDRNetwork, IPAddress>, 2> networkAndNexthops = {
      std::make_pair(
          CIDRNetwork(IPAddress("20.1.1.0"), 32), IPAddress("2.2.2.10")),
      std::make_pair(
          CIDRNetwork(IPAddress("2001::0"), 128), IPAddress("2::10"))};
  auto setup = [=]() {
    applyNewConfig(initialConfig());
    auto newRouteTables = getProgrammedState()->getRouteTables();
    for (const auto& networkAndNexthop : networkAndNexthops) {
      newRouteTables = utility::addRoute(
          newRouteTables, networkAndNexthop.first, networkAndNexthop.second);
      CHECK(newRouteTables != nullptr);
    }
    auto newState = getProgrammedState()->clone();
    newState->resetRouteTables(newRouteTables);
    applyNewState(newState);
  };
  auto verify = [=]() {
    for (const auto& networkAndNexthop : networkAndNexthops) {
      auto* hostRoute = getHwSwitch()->getHostTable()->getBcmHost(BcmHostKey(
          getHwSwitch()->getBcmVrfId(utility::kRouter0),
          networkAndNexthop.first.first));
      // Host route should not set dst class l3 id
      checkBcmHostMatchInHw(hostRoute, false);
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
