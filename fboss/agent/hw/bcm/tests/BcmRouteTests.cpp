/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/bcm/tests/BcmTestRouteUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <boost/range/combine.hpp>
#include <folly/logging/xlog.h>

#include <string>

extern "C" {
#include <bcm/l3.h>
}

using folly::ByteRange;
using folly::CIDRNetwork;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using std::make_shared;
using std::shared_ptr;
using std::string;
using namespace facebook::fboss;
using facebook::fboss::utility::kBaseVlanId;

namespace {
int L3EgressPrintCb(
    int unit,
    bcm_if_t intf,
    bcm_l3_egress_t* info,
    void* /*user_data*/) {
  XLOG(DBG3) << "L3EgressEntry in unit : " << unit << " intf : " << intf
             << " flag : " << std::showbase << std::hex << info->flags
             << " info->intf : " << std::showbase << info->intf;
  return 0;
}

int L3RouteCountCb(
    int unit,
    int /*index*/,
    bcm_l3_route_t* info,
    void* user_data) {
  int* count = static_cast<int*>(user_data);
  *count += 1;

  auto ip = info->l3a_flags & BCM_L3_IP6
      ? IPAddress::fromBinary(
            ByteRange(info->l3a_ip6_net, sizeof(info->l3a_ip6_net)))
      : IPAddress::fromLongHBO(info->l3a_subnet);
  auto mask = info->l3a_flags & BCM_L3_IP6
      ? IPAddress::fromBinary(
            ByteRange(info->l3a_ip6_mask, sizeof(info->l3a_ip6_mask)))
      : IPAddress::fromLongHBO(info->l3a_ip_mask);
  auto intf = info->l3a_intf;

  bcm_l3_egress_t egr;
  bcm_l3_egress_t_init(&egr);
  auto rv = bcm_l3_egress_get(unit, intf, &egr);
  bcmCheckError(rv, "failed to get l4 egress");

  bcm_l3_intf_t intfInfo;
  bcm_l3_intf_t_init(&intfInfo);
  intfInfo.l3a_intf_id = egr.intf;
  rv = bcm_l3_intf_get(unit, &intfInfo);
  bcmCheckError(rv, folly::sformat("failed to get l3 intf: {}", egr.intf));

  auto mac = MacAddress::fromBinary(
      ByteRange(intfInfo.l3a_mac_addr, MacAddress::SIZE));
  auto vlan = intfInfo.l3a_vid;

  XLOG(DBG3) << "L3RouteEntry in vrf : " << info->l3a_vrf
             << " route for : " << ip << " mask: " << mask
             << " egr.intf: " << std::showbase << std::hex << egr.intf
             << std::dec << " intf: " << intf << " mac: " << mac
             << " vlan: " << vlan;

  return 0;
}

int L3RouteToCPUCb(
    int unit,
    int /*index*/,
    bcm_l3_route_t* info,
    void* user_data) {
  bool* allToCPU = static_cast<bool*>(user_data);
  auto intf = info->l3a_intf;
  bcm_l3_egress_t egr;
  auto rv = bcm_l3_egress_get(unit, intf, &egr);
  bcmCheckError(rv, "failed to get l3 egress");

  if ((egr.flags & BCM_L3_L2TOCPU) == 0 &&
      (egr.flags & BCM_L3_DST_DISCARD) == 0) {
    XLOG(INFO) << "egress " << intf
               << " does not have BCM_L3_L2TOCPU or DISCARD flag";
    *allToCPU = false;
  }

  return 0;
}

} // namespace

using IncomingPacketCb =
    folly::Function<void(PortID, std::unique_ptr<folly::IOBuf>)>;

namespace facebook::fboss {

IncomingPacketCb FakeSdk_getIncomingPacketCb();

class BcmRouteTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::twoL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }

  std::shared_ptr<RouteTableMap> publishRoutes(
      std::shared_ptr<RouteTableMap> routeTables) {
    if (nullptr != routeTables) {
      auto newState = getProgrammedState()->clone();
      newState->resetRouteTables(routeTables);
      applyNewState(newState)->getRouteTables();
    }
    return routeTables;
  }

  BcmHostKey getHostKey(
      IPAddress address,
      RouterID routerID = utility::kRouter0) {
    return BcmHostKey(getHwSwitch()->getBcmVrfId(routerID), address);
  }

  std::tuple<int, int> computeNonHostInterfaceRoutes() {
    int ipv4Routes, ipv6Routes;

    ipv4Routes = ipv6Routes = 0;
    for (auto interface : *getProgrammedState()->getInterfaces()) {
      for (auto address : interface->getAddresses()) {
        auto mask = address.second;
        auto ipAddress = address.first;

        if (ipAddress.isV4() && mask != 32) {
          ipv4Routes++;
        }
        if (ipAddress.isV6() && !ipAddress.isLinkLocal() && mask != 128) {
          ipv6Routes++;
        }
      }
    }

    return std::make_tuple(ipv4Routes, ipv6Routes);
  }

  long referenceCount(const BcmMultiPathNextHopKey& key) {
    return getHwSwitch()
        ->getMultiPathNextHopTable()
        ->getNextHops()
        .referenceCount(key);
  }

  void routeReferenceCountTest(
      const CIDRNetwork& network,
      const IPAddress& nexthop);

  void verifyBcmHostReference(bcm_vrf_t vrf, const RouteNextHopSet& nexthops);

  void verifyNextHopReferences(const std::vector<IPAddress>& nexthops);
};

class BcmRouteHostReferenceTest : public BcmRouteTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }

  /*
   * A key in host table, based on next hop and interface id
   */
  BcmMultiPathNextHopKey getKey(
      IPAddress nexthop,
      int interface,
      NextHopWeight weight = UCMP_DEFAULT_WEIGHT) {
    return BcmMultiPathNextHopKey(
        utility::kRouter0,
        RouteNextHopEntry(
            ResolvedNextHop(nexthop, InterfaceID(interface), weight),
            utility::DISTANCE)
            .getNextHopSet());
  }

  long referenceCount(const BcmMultiPathNextHopKey& key) {
    return getHwSwitch()
        ->getMultiPathNextHopTable()
        ->getNextHops()
        .referenceCount(key);
  }
};

} // namespace facebook::fboss

TEST_F(BcmRouteTest, AddRoutesAndCountPrintCheckToCPU) {
  auto setup = [=]() {
    applyNewConfig(initialConfig());
    std::vector<IPAddress> nexthops = {
        IPAddress("1.1.1.10"),
        IPAddress("2.2.2.10"),
        IPAddress("1::10"),
        IPAddress("2::10"),
    };
    std::vector<CIDRNetwork> networks = {
        CIDRNetwork(IPAddress("10.1.1.0"), 24),
        CIDRNetwork(IPAddress("20.1.1.0"), 24),
        CIDRNetwork(IPAddress("1001::0"), 48),
        CIDRNetwork(IPAddress("2001::0"), 48),
    };

    shared_ptr<RouteTableMap> routeTables =
        getProgrammedState()->getRouteTables();
    for (auto tuple : boost::combine(networks, nexthops)) {
      auto network = tuple.get<0>();
      auto nexthop = tuple.get<1>();
      routeTables = utility::addRoute(routeTables, network, nexthop);
    }
    routeTables = publishRoutes(routeTables);
  };

  auto verify = [=]() {
    bcm_l3_info_t hwStatus;
    auto rv = bcm_l3_info(getUnit(), &hwStatus);
    bcmCheckError(rv, "failed get L3 hw info");
    bcm_l3_egress_traverse(getUnit(), L3EgressPrintCb, nullptr);

    uint32_t start = 0, end = hwStatus.l3info_max_route;
    int routeCount = 0; // number of routes

    bcm_l3_route_traverse(
        getUnit(), 0, start, end, L3RouteCountCb, &routeCount);
    bcm_l3_route_traverse(
        getUnit(), BCM_L3_IP6, start, end >> 1, L3RouteCountCb, &routeCount);

    // 4 + 4 + 1 fe80::/64 link local address
    // 4 : 2 routes (v4 & v6) per interface for 2 interfaces
    // 4 : 4 routes added in the test
    // 1 : 1 route for link local addresses
    EXPECT_EQ(9 + numMinAlpmRoutes(), routeCount);

    // Check next hops (CPU)
    // So far we could not verify the src/dst MAC address for a specific route
    // entry, because the first packet will be punted to the CPU before the ARP
    // resolution.
    // All newly created egress object will point to intf ID 4096, with
    // BCM_L3_L2TOCPU bit set
    bool allToCPU = true;
    bcm_l3_route_traverse(getUnit(), 0, start, end, L3RouteToCPUCb, &allToCPU);
    bcm_l3_route_traverse(
        getUnit(), BCM_L3_IP6, start, end, L3RouteToCPUCb, &allToCPU);
    EXPECT_TRUE(allToCPU);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmRouteHostReferenceTest, AddRouteAndCheckHostReference) {
  int intf1 = kBaseVlanId; /* config has one interface */
  std::vector<IPAddress> nexthops = {
      IPAddress("1.1.1.101"),
      IPAddress("1::101"),
  };
  auto setup = [=]() {
    applyNewConfig(initialConfig());
    shared_ptr<RouteTableMap> routeTables =
        getProgrammedState()->getRouteTables();
    std::vector<CIDRNetwork> networks = {
        CIDRNetwork(IPAddress("10.1.1.0"), 24),
        CIDRNetwork(IPAddress("1001:1::"), 48),
    };

    /* add routes */
    for (auto tuple : boost::combine(networks, nexthops)) {
      auto network = tuple.get<0>();
      auto nexthop = tuple.get<1>();
      routeTables = utility::addRoute(routeTables, network, nexthop);
    }
    routeTables = publishRoutes(routeTables);
  };

  auto verify = [=]() {
    /* basic test, each next hop should be reachable only from one network, so
     * reference count should be one */
    for (auto nexthop : nexthops) {
      EXPECT_EQ(referenceCount(getKey(nexthop, intf1)), 1);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(
    BcmRouteHostReferenceTest,
    AddRoutesWithDistinctNextHopsAndCheckHostReference) {
  int intf1 = kBaseVlanId; /* config has one interface */
  std::vector<IPAddress> nexthops = {
      IPAddress("1.1.1.101"),
      IPAddress("1.1.1.102"),
      IPAddress("1::101"),
      IPAddress("1::102"),
  };
  auto setup = [=]() {
    applyNewConfig(initialConfig());
    std::vector<CIDRNetwork> networks = {
        CIDRNetwork(IPAddress("10.1.1.0"), 24),
        CIDRNetwork(IPAddress("10.1.2.0"), 24),
        CIDRNetwork(IPAddress("1001:1::"), 48),
        CIDRNetwork(IPAddress("1001:2::"), 48),
    };
    shared_ptr<RouteTableMap> routeTables =
        getProgrammedState()->getRouteTables();
    /* add routes */
    for (auto tuple : boost::combine(networks, nexthops)) {
      auto network = tuple.get<0>();
      auto nexthop = tuple.get<1>();
      routeTables = utility::addRoute(routeTables, network, nexthop);
    }
    routeTables = publishRoutes(routeTables);
  };
  auto verify = [=]() {
    /* each next hop is reached from only one route, so reference count for each
     * next hop should be 1 */
    for (auto nexthop : nexthops) {
      EXPECT_EQ(referenceCount(getKey(nexthop, intf1)), 1);
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(
    BcmRouteHostReferenceTest,
    AddRoutesWithSharedNextHopsAndCheckHostReference) {
  int intf1 = kBaseVlanId; /* config has one interface */
  std::vector<IPAddress> nexthops = {
      IPAddress("1.1.1.101"),
      IPAddress("1.1.1.101"), /* two V4 networks have same nexthop */
      IPAddress("1::1"),
      IPAddress("1::1"), /* two V6 networks have same nexthop */
  };
  auto setup = [=]() {
    applyNewConfig(initialConfig());
    shared_ptr<RouteTableMap> routeTables =
        getProgrammedState()->getRouteTables();
    std::vector<CIDRNetwork> networks = {
        CIDRNetwork(IPAddress("10.1.1.0"), 24),
        CIDRNetwork(IPAddress("10.1.2.0"), 24),
        CIDRNetwork(IPAddress("1001:1::"), 48),
        CIDRNetwork(IPAddress("1001:2::"), 48),
    };

    /* add routes */
    for (auto tuple : boost::combine(networks, nexthops)) {
      auto network = tuple.get<0>();
      auto nexthop = tuple.get<1>();
      routeTables = utility::addRoute(routeTables, network, nexthop);
    }
    routeTables = publishRoutes(routeTables);
  };

  auto verify = [=]() {
    /* Verify reference counts for each nexthop is 2 (as two networks share
     * each) */
    for (auto nexthop : nexthops) {
      EXPECT_EQ(referenceCount(getKey(nexthop, intf1)), 2);
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmRouteHostReferenceTest, DeleteRoutesAndCheckHostReference) {
  int intf1 = kBaseVlanId; /* config has one interface */
  auto setup = [=]() {
    applyNewConfig(initialConfig());
    shared_ptr<RouteTableMap> routeTables =
        getProgrammedState()->getRouteTables();
    std::vector<IPAddress> nexthops = {
        IPAddress("1.1.1.101"),
        IPAddress("1::1"),
        IPAddress("1.1.1.101"), /* two V4 networks share same nexthop */
        IPAddress("1::1"), /* two V6 networks share same nexthop */
    };
    std::vector<CIDRNetwork> networks = {
        CIDRNetwork(IPAddress("10.1.1.0"), 24),
        CIDRNetwork(IPAddress("1001:1::"), 48),
        CIDRNetwork(IPAddress("10.1.2.0"), 24),
        CIDRNetwork(IPAddress("1001:2::"), 48),
    };

    /* add route */
    for (auto tuple : boost::combine(networks, nexthops)) {
      auto network = tuple.get<0>();
      auto nexthop = tuple.get<1>();
      routeTables = utility::addRoute(routeTables, network, nexthop);
    }
    routeTables = publishRoutes(routeTables);
    for (auto nexthop : nexthops) {
      EXPECT_EQ(referenceCount(getKey(nexthop, intf1)), 2);
    }

    /* now remove one network per next hop */
    networks.pop_back();
    nexthops.pop_back();
    networks.pop_back();
    nexthops.pop_back();

    for (auto network : networks) {
      routeTables = utility::deleteRoute(routeTables, network);
    }
    routeTables = publishRoutes(routeTables);
  };

  auto verify = [=]() {
    std::vector<IPAddress> nexthops = {IPAddress("1.1.1.101"),
                                       IPAddress("1::1")};
    /* as route to one network for each next hop will be removed, verify
     * reference count for each next hop is 1*/
    for (auto nexthop : nexthops) {
      EXPECT_EQ(referenceCount(getKey(nexthop, intf1)), 1);
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmRouteHostReferenceTest, AddRouteAndCheckInvalidHostReference) {
  int intf1 = kBaseVlanId; /* config has one interface */
  int intf2 = 2; /* unrelated interface */

  std::vector<IPAddress> nexthops = {
      IPAddress("1.1.1.101"),
      IPAddress("1::101"),
  };
  auto setup = [=]() {
    applyNewConfig(initialConfig());
    shared_ptr<RouteTableMap> routeTables =
        getProgrammedState()->getRouteTables();
    std::vector<CIDRNetwork> networks = {
        CIDRNetwork(IPAddress("10.1.1.0"), 24),
        CIDRNetwork(IPAddress("1001:1::"), 48),
    };

    /* add routes */
    for (auto tuple : boost::combine(networks, nexthops)) {
      auto network = tuple.get<0>();
      auto nexthop = tuple.get<1>();
      routeTables = utility::addRoute(routeTables, network, nexthop);
    }
    routeTables = publishRoutes(routeTables);
  };

  auto verify = [=]() {
    /* nexthop is not resolved through through intf2, so reference count should
     * be zero */
    for (auto nexthop : nexthops) {
      EXPECT_EQ(referenceCount(getKey(nexthop, intf2)), 0);
    }
    /* unknown nexthops should have reference count zero */
    EXPECT_EQ(referenceCount(getKey(IPAddress("1.1.1.102"), intf1)), 0);
    EXPECT_EQ(referenceCount(getKey(IPAddress("1::102"), intf2)), 0);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmRouteHostReferenceTest, AddNoRoutesAndCheckDefaultHostReference) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    std::vector<IPAddress> interfaceIps;
    std::vector<int32_t> interfaceIds;

    /* entries are created for non-local IP addresses of interface */
    for (auto interface : *getProgrammedState()->getInterfaces()) {
      for (auto address : interface->getAddresses()) {
        if (address.first.isV6() && address.first.isLinkLocal()) {
          continue;
        }
        interfaceIps.push_back(address.first);
        interfaceIds.push_back(interface->getID());
      }
    }
    /* verify reference count is one by default for interface address */
    for (auto tuple : boost::combine(interfaceIds, interfaceIps)) {
      auto intfId = tuple.get<0>();
      auto intfIp = tuple.get<1>();
      EXPECT_EQ(referenceCount(getKey(intfIp, intfId, 1)), 1);
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

namespace {

int getHwRouteCount(
    int unit,
    const IPAddress& networkIP,
    bcm_l3_info_t hwStatus) {
  int routeCount = 0;
  auto routeTraverseCb = [](int, int, bcm_l3_route_t*, void* user_data) {
    int* count = static_cast<int*>(user_data);
    *count += 1;

    return 0;
  };
  auto rv = bcm_l3_route_traverse(
      unit,
      networkIP.isV4() ? 0 : BCM_L3_IP6,
      0,
      hwStatus.l3info_max_route,
      routeTraverseCb,
      &routeCount);
  bcmCheckError(rv, "failed to l3 route traverse");
  return routeCount;
}

int getHwHostRouteCount(
    int unit,
    const IPAddress& networkIP,
    bcm_l3_info_t hwStatus) {
  int hostRouteCount = 0;
  auto hostTraverseCb = [](int, int, bcm_l3_host_t*, void* user_data) {
    int* count = static_cast<int*>(user_data);
    *count += 1;

    return 0;
  };
  auto rv = bcm_l3_host_traverse(
      unit,
      networkIP.isV4() ? 0 : BCM_L3_IP6,
      0,
      hwStatus.l3info_max_host,
      hostTraverseCb,
      &hostRouteCount);
  bcmCheckError(rv, "failed to l3 host traverse");
  return hostRouteCount;
}

int getHwRoute(int unit, const IPAddress& networkIP, uint8_t netmask) {
  bcm_l3_route_t route;
  bcm_l3_route_t_init(&route);
  if (networkIP.isV4()) {
    route.l3a_subnet = networkIP.asV4().toLongHBO();
    route.l3a_ip_mask =
        folly::IPAddressV4(folly::IPAddressV4::fetchMask(netmask)).toLongHBO();
  } else { // IPv6
    ipToBcmIp6(networkIP, &route.l3a_ip6_net);
    memcpy(
        &route.l3a_ip6_mask,
        folly::IPAddressV6::fetchMask(netmask).data(),
        sizeof(route.l3a_ip6_mask));
    route.l3a_flags = BCM_L3_IP6;
  }

  return bcm_l3_route_get(unit, &route);
}

int getHwRoute(
    int unit,
    const IPAddress& networkIP,
    uint8_t netmask,
    bcm_l3_route_t& route) {
  bcm_l3_route_t_init(&route);
  if (networkIP.isV4()) {
    route.l3a_subnet = networkIP.asV4().toLongHBO();
    route.l3a_ip_mask =
        folly::IPAddressV4(folly::IPAddressV4::fetchMask(netmask)).toLongHBO();
  } else { // IPv6
    ipToBcmIp6(networkIP, &route.l3a_ip6_net);
    memcpy(
        &route.l3a_ip6_mask,
        folly::IPAddressV6::fetchMask(netmask).data(),
        sizeof(route.l3a_ip6_mask));
    route.l3a_flags = BCM_L3_IP6;
  }

  return bcm_l3_route_get(unit, &route);
}

int findHwHost(int unit, const IPAddress& networkIP, bcm_l3_host_t& host) {
  bcm_l3_host_t_init(&host);
  if (networkIP.isV4()) {
    host.l3a_ip_addr = networkIP.asV4().toLongHBO();
  } else { // IPv6
    ipToBcmIp6(networkIP, &host.l3a_ip6_addr);
    host.l3a_flags = BCM_L3_IP6;
  }

  return bcm_l3_host_find(unit, &host);
}

} // unnamed namespace

void BcmRouteTest::routeReferenceCountTest(
    const CIDRNetwork& network,
    const IPAddress& nexthop) {
  auto setup = [=]() {
    applyNewConfig(initialConfig());
    shared_ptr<RouteTableMap> routeTables =
        getProgrammedState()->getRouteTables();
    routeTables = utility::addRoute(routeTables, network, nexthop);
    routeTables = publishRoutes(routeTables);
  };

  auto verify = [=]() {
    const IPAddress& networkIP = network.first;
    const uint8_t netmask = network.second;
    EXPECT_TRUE(networkIP.isV4() || networkIP.isV6());
    int v4NonHostInterfaceRoutes, v6NonHostInterfaceRoutes;
    std::tie(v4NonHostInterfaceRoutes, v6NonHostInterfaceRoutes) =
        computeNonHostInterfaceRoutes();
    auto nonHostInterfaceRoutes =
        networkIP.isV4() ? v4NonHostInterfaceRoutes : v6NonHostInterfaceRoutes;
    auto numAlpmRoutes =
        networkIP.isV4() ? numMinAlpmV4Routes() : numMinAlpmV6Routes();
    bool isHostRouteAndCanUseHostTable = networkIP.bitCount() == netmask &&
        getPlatform()->canUseHostTableForHostRoutes();
    auto hostTable = getHwSwitch()->getHostTable();

    EXPECT_EQ(
        isHostRouteAndCanUseHostTable ? 1 : 0,
        hostTable->getReferenceCount(getHostKey(networkIP)));

    bcm_l3_info_t hwStatus;
    auto rv = bcm_l3_info(getUnit(), &hwStatus);
    bcmCheckError(rv, "failed to get L3 hw info");

    auto routeCount = getHwRouteCount(getUnit(), networkIP, hwStatus);
    auto interfaceAndLinkLocalRoutes = nonHostInterfaceRoutes +
        (networkIP.isV4() ? 0 : 1); // For v6 we add fe80 link local routes
    EXPECT_EQ(
        interfaceAndLinkLocalRoutes + numAlpmRoutes +
            (isHostRouteAndCanUseHostTable ? 0 : 1),
        routeCount);

    auto hostRouteCount = getHwHostRouteCount(getUnit(), networkIP, hwStatus);
    EXPECT_EQ(
        nonHostInterfaceRoutes + 1 + // nexthop
            (isHostRouteAndCanUseHostTable ? 1 : 0),
        hostRouteCount);

    // A non-host route, host route if we can;t use host table for host routes
    // should be present in hardware route table
    EXPECT_EQ(
        isHostRouteAndCanUseHostTable ? BCM_E_NOT_FOUND : 0,
        getHwRoute(getUnit(), networkIP, netmask));
  };

  verifyAcrossWarmBoots(setup, verify);
}

void BcmRouteTest::verifyBcmHostReference(
    bcm_vrf_t vrf,
    const RouteNextHopSet& nexthops) {
  auto count =
      referenceCount(std::make_pair(vrf, nexthops)); // initial ref count

  auto* ecmpHost = getHwSwitch()->getMultiPathNextHopTable()->getNextHopIf(
      BcmMultiPathNextHopKey(vrf, nexthops));
  EXPECT_NE(ecmpHost, nullptr);

  EXPECT_EQ(referenceCount(std::make_pair(vrf, nexthops)), count);
}

void BcmRouteTest::verifyNextHopReferences(
    const std::vector<IPAddress>& nexthops) {
  for (auto& nexthop : nexthops) {
    if (nexthop.isV6()) {
      const auto& route = getProgrammedState()
                              ->getRouteTables()
                              ->getRouteTable(RouterID(0))
                              ->getRibV6()
                              ->longestMatch(nexthop.asV6());
      EXPECT_EQ(route->isResolved(), true);
      const auto& fwd = route->getForwardInfo();
      verifyBcmHostReference(0, fwd.normalizedNextHops());

    } else {
      const auto& route = getProgrammedState()
                              ->getRouteTables()
                              ->getRouteTable(RouterID(0))
                              ->getRibV4()
                              ->longestMatch(nexthop.asV4());
      EXPECT_EQ(route->isResolved(), true);
      const auto& fwd = route->getForwardInfo();
      verifyBcmHostReference(0, fwd.normalizedNextHops());
    }
  }
}
TEST_F(BcmRouteTest, AddV4NonHostRoute) {
  routeReferenceCountTest(
      CIDRNetwork(IPAddress("10.1.1.0"), 24), IPAddress("1.1.1.10"));
}

TEST_F(BcmRouteTest, AddV4HostRoute) {
  routeReferenceCountTest(
      CIDRNetwork(IPAddress("20.1.1.0"), 32), IPAddress("2.2.2.10"));
}

TEST_F(BcmRouteTest, AddV6NonHostRoute) {
  routeReferenceCountTest(
      CIDRNetwork(IPAddress("1001::0"), 48), IPAddress("1::10"));
}

TEST_F(BcmRouteTest, AddV6HostRoute) {
  routeReferenceCountTest(
      CIDRNetwork(IPAddress("2001::0"), 128), IPAddress("2::10"));
}

TEST_F(BcmRouteTest, HostRouteStat) {
  HwResourceStats preUpdateStat;
  auto setup = [&]() {
    applyNewConfig(initialConfig());
    preUpdateStat = getHwSwitch()->getStatUpdater()->getHwTableStats();
    auto network = CIDRNetwork(IPAddress("10.1.1.10"), 32);
    auto nexthop = IPAddress("1.1.1.10");
    shared_ptr<RouteTableMap> routeTables =
        getProgrammedState()->getRouteTables();
    routeTables = utility::addRoute(routeTables, network, nexthop);
    routeTables = publishRoutes(routeTables);
  };

  auto verify = [&]() {
    HwResourceStats postUpdateStat;
    postUpdateStat = getHwSwitch()->getStatUpdater()->getHwTableStats();
    auto expectedV4HostRouteIncrement =
        getPlatform()->canUseHostTableForHostRoutes() ? 2 /*host route + nhop*/
                                                      : 1 /*nhop*/;
    EXPECT_EQ(
        *postUpdateStat.l3_host_used_ref(),
        *preUpdateStat.l3_host_used_ref() + expectedV4HostRouteIncrement);
    EXPECT_EQ(
        *postUpdateStat.l3_ipv4_host_used_ref(),
        *preUpdateStat.l3_ipv4_host_used_ref() + expectedV4HostRouteIncrement);
    EXPECT_EQ(
        *postUpdateStat.l3_ipv6_host_used_ref(),
        *preUpdateStat.l3_ipv6_host_used_ref());
  };
  setup();
  verify();
}

/* TODO (pshaikh) T36690958
TEST_F(BcmRouteTest, LpmRouteV6Stat64b) {
  HwResourceStats preUpdateStat;
  auto setup = [&]() {
    applyNewConfig(initialConfig());
    preUpdateStat = getHwSwitch()->getStatUpdater()->getHwTableStats();
    auto network = CIDRNetwork(IPAddress("1001::0"), 48);
    auto nexthop = IPAddress("1::10");
    shared_ptr<RouteTableMap> routeTables =
      getProgrammedState()->getRouteTables();
    routeTables = utility::addRoute(routeTables, network, nexthop);
    routeTables = publishRoutes(routeTables);

  };

  auto verify = [&]() {
    HwResourceStats postUpdateStat;
    postUpdateStat = getHwSwitch()->getStatUpdater()->getHwTableStats();
    EXPECT_EQ(postUpdateStat.lpm_ipv4_used, preUpdateStat.lpm_ipv4_used);
    EXPECT_EQ(
        postUpdateStat.lpm_ipv6_mask_0_64_used,
        preUpdateStat.lpm_ipv6_mask_0_64_used + 1);
    EXPECT_EQ(
        postUpdateStat.lpm_ipv6_mask_65_127_used,
        preUpdateStat.lpm_ipv6_mask_65_127_used);
  };
  setup();
  verify();
}*/

TEST_F(BcmRouteTest, LpmRouteV6Stat128b) {
  HwResourceStats preUpdateStat;
  auto setup = [&]() {
    applyNewConfig(initialConfig());
    preUpdateStat = getHwSwitch()->getStatUpdater()->getHwTableStats();
    auto network = CIDRNetwork(IPAddress("1001::0"), 127);
    auto nexthop = IPAddress("1::10");
    shared_ptr<RouteTableMap> routeTables =
        getProgrammedState()->getRouteTables();
    routeTables = utility::addRoute(routeTables, network, nexthop);
    routeTables = publishRoutes(routeTables);
  };

  auto verify = [&]() {
    HwResourceStats postUpdateStat;
    postUpdateStat = getHwSwitch()->getStatUpdater()->getHwTableStats();
    EXPECT_EQ(
        *postUpdateStat.lpm_ipv4_used_ref(),
        *preUpdateStat.lpm_ipv4_used_ref());
    EXPECT_EQ(
        *postUpdateStat.lpm_ipv6_mask_0_64_used_ref(),
        *preUpdateStat.lpm_ipv6_mask_0_64_used_ref());
    EXPECT_EQ(
        *postUpdateStat.lpm_ipv6_mask_65_127_used_ref(),
        *preUpdateStat.lpm_ipv6_mask_65_127_used_ref() + 1);
  };
  setup();
  verify();
}

TEST_F(BcmRouteTest, BcmHostReference) {
  std::vector<IPAddress> v4Nexthops = {
      IPAddress("1.1.1.101"),
      IPAddress("1.1.1.102"),
      IPAddress("1.1.1.103"),
      IPAddress("1.1.1.104"),
  };

  std::vector<IPAddress> v6Nexthops = {
      IPAddress("1::101"),
      IPAddress("1::102"),
      IPAddress("1::103"),
      IPAddress("1::104"),
  };

  auto v4Network = CIDRNetwork(IPAddress("10.1.2.0"), 24);
  auto v6Network = CIDRNetwork(IPAddress("1001:2::"), 48);

  auto setup = [=]() {
    // add multi path route
    applyNewConfig(initialConfig());
    shared_ptr<RouteTableMap> routeTables =
        getProgrammedState()->getRouteTables();
    /* add routes */
    routeTables = utility::addRoute(routeTables, v4Network, v4Nexthops);
    routeTables = utility::addRoute(routeTables, v6Network, v6Nexthops);

    routeTables = publishRoutes(routeTables);
  };
  auto verify = [=]() {
    verifyNextHopReferences(v4Nexthops);
    verifyNextHopReferences(v6Nexthops);
    verifyNextHopReferences({IPAddress("10.1.2.100")});
    verifyNextHopReferences({IPAddress("1001:2::10")});
  };
  setup();
  verify();
}

TEST_F(BcmRouteTest, EgressUpdateOnHostRouteUpdateOneHopToManyHops) {
  auto hostRouteV4 = CIDRNetwork{folly::IPAddressV4("101.102.103.104"), 32};
  auto hostRouteV6 = CIDRNetwork{folly::IPAddressV6("2401::dead:beef"), 128};
  std::vector<IPAddress> v4Nexthops = {
      IPAddress("1.1.1.101"),
      IPAddress("2.2.2.101"),
  };

  std::vector<IPAddress> v6Nexthops = {
      IPAddress("1::101"),
      IPAddress("2::101"),
  };

  auto setup = [=]() {
    // add multi path route
    auto config = initialConfig();
    applyNewConfig(config);
    for (auto i = 0; i < 2; i++) {
      auto portId = masterLogicalPortIds()[i];
      auto helper4 = utility::EcmpSetupTargetedPorts<IPAddressV4>(
          getProgrammedState(), RouterID(0));
      applyNewState(helper4.resolveNextHops(
          getProgrammedState(), {PortDescriptor(portId)}));

      auto helper6 = utility::EcmpSetupTargetedPorts<IPAddressV6>(
          getProgrammedState(), RouterID(0));
      applyNewState(helper6.resolveNextHops(
          getProgrammedState(), {PortDescriptor(portId)}));
    }

    // host route has only one next hop
    auto rtables0 = getProgrammedState()->getRouteTables();
    rtables0 = utility::addRoute(rtables0, hostRouteV4, v4Nexthops[0]);
    rtables0 = utility::addRoute(rtables0, hostRouteV6, v6Nexthops[0]);
    auto state0 = getProgrammedState()->clone();
    state0->resetRouteTables(rtables0);
    auto state1 = applyNewState(state0);

    auto rtables1 = state1->getRouteTables();
    // make it ECMP egress
    rtables1 = utility::addRoute(rtables1, hostRouteV4, v4Nexthops);
    rtables1 = utility::addRoute(rtables1, hostRouteV6, v6Nexthops);

    state1 = state1->clone();
    state1->resetRouteTables(rtables1);

    applyNewState(state1);
  };

  auto verify = [=]() {
    auto v4SwRoute = getProgrammedState()
                         ->getRouteTables()
                         ->getRouteTable(RouterID(0))
                         ->getRibV4()
                         ->routes()
                         ->getRouteIf(RoutePrefixV4{hostRouteV4.first.asV4(),
                                                    hostRouteV4.second});

    auto v6SwRoute = getProgrammedState()
                         ->getRouteTables()
                         ->getRouteTable(RouterID(0))
                         ->getRibV6()
                         ->routes()
                         ->getRouteIf(RoutePrefixV6{hostRouteV6.first.asV6(),
                                                    hostRouteV6.second});

    ASSERT_NE(v4SwRoute, nullptr);
    ASSERT_NE(v6SwRoute, nullptr);
    ASSERT_EQ(v4SwRoute->isResolved(), true);
    ASSERT_EQ(v6SwRoute->isResolved(), true);

    auto* v4BcmRoute = getHwSwitch()->routeTable()->getBcmRoute(
        0, hostRouteV4.first, hostRouteV4.second);
    auto* v6BcmRoute = getHwSwitch()->routeTable()->getBcmRoute(
        0, hostRouteV6.first, hostRouteV6.second);

    if (getPlatform()->canUseHostTableForHostRoutes()) {
      bcm_l3_host_t v4HostRoute;
      bcm_l3_host_t v6HostRoute;

      findHwHost(0, hostRouteV4.first, v4HostRoute);
      findHwHost(0, hostRouteV6.first, v6HostRoute);

      ASSERT_EQ(v4BcmRoute->getEgressId(), v4HostRoute.l3a_intf);
      ASSERT_EQ(v6BcmRoute->getEgressId(), v6HostRoute.l3a_intf);
    } else {
      bcm_l3_route_t v4HostRoute;
      bcm_l3_route_t v6HostRoute;

      getHwRoute(0, hostRouteV4.first, hostRouteV4.second, v4HostRoute);
      getHwRoute(0, hostRouteV6.first, hostRouteV6.second, v6HostRoute);

      ASSERT_EQ(v4BcmRoute->getEgressId(), v4HostRoute.l3a_intf);
      ASSERT_EQ(v6BcmRoute->getEgressId(), v6HostRoute.l3a_intf);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmRouteTest, UnresolveResolveNextHop) {
  auto config = initialConfig();
  boost::container::flat_set<PortDescriptor> ports;
  for (auto i = 0; i < 2; i++) {
    auto portId = masterLogicalPortIds()[i];
    ports.insert(PortDescriptor(portId));
  }
  auto route = RoutePrefixV6{folly::IPAddressV6("2401:dead:beef::"), 112};

  auto setup = [=]() {
    // add multi path route
    applyNewConfig(config);
    auto helper = utility::EcmpSetupTargetedPorts<IPAddressV6>(
        getProgrammedState(), RouterID(0));

    applyNewState(helper.resolveNextHops(getProgrammedState(), ports));

    std::map<PortDescriptor, std::shared_ptr<NdpEntry>> entries;

    // mark neighbors connected over ports pending
    auto state0 = getProgrammedState();
    for (auto port : ports) {
      auto ecmpNextHop = helper.nhop(port);
      auto vlanId = helper.getVlan(port);
      auto ntable = state0->getVlans()->getVlan(*vlanId)->getNdpTable()->modify(
          *vlanId, &state0);
      auto entry = ntable->getEntry(ecmpNextHop.ip);
      auto intfId = entry->getIntfID();
      ntable->removeEntry(ecmpNextHop.ip);
      ntable->addPendingEntry(ecmpNextHop.ip, intfId);
      entries[port] = std::move(entry);
    }
    applyNewState(state0);

    // mark neighbors connected over ports reachable
    auto state1 = getProgrammedState();
    for (auto port : ports) {
      auto vlanId = helper.getVlan(port);
      auto ntable = state1->getVlans()->getVlan(*vlanId)->getNdpTable()->modify(
          *vlanId, &state1);
      auto entry = entries[port];
      auto* fields = entry->getFields();
      ntable->updateEntry(*fields);
    }
    applyNewState(state1);

    applyNewState(
        helper.setupECMPForwarding(getProgrammedState(), ports, {route}));
  };
  auto verify = [=]() {
    /* route is programmed */
    auto* bcmRoute =
        getHwSwitch()->routeTable()->getBcmRoute(0, route.network, route.mask);
    EXPECT_NE(bcmRoute, nullptr);
    auto egressId = bcmRoute->getEgressId();
    EXPECT_NE(egressId, BcmEgressBase::INVALID);

    /* ecmp is resolved */
    bcm_l3_egress_ecmp_t ecmp_info;
    bcm_l3_egress_ecmp_t_init(&ecmp_info);
    ecmp_info.ecmp_intf = egressId;
    std::array<bcm_if_t, 2> ecmp_member_array;
    int ecmp_member_count = 0;
    bcm_l3_egress_ecmp_get(
        getHwSwitch()->getUnit(),
        &ecmp_info,
        ecmp_member_array.size(),
        ecmp_member_array.data(),
        &ecmp_member_count);
    EXPECT_EQ(ecmp_member_count, ecmp_member_array.size());
  };
  setup();
  verify();
}
