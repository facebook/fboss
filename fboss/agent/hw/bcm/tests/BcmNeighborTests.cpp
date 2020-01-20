// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/TrunkUtils.h"

using namespace ::testing;

extern "C" {
#include <bcm/l3.h>
}

namespace {

template <typename AddrType, bool trunk = false>
struct NeighborT {
  using IPAddrT = AddrType;
  static constexpr auto isTrunk = trunk;
  using LinkT = std::conditional_t<isTrunk, bcm_trunk_t, bcm_port_t>;
  static facebook::fboss::cfg::SwitchConfig initialConfig(
      facebook::fboss::cfg::SwitchConfig config);

  template <typename AddrT = IPAddrT>
  std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      folly::IPAddressV4> static getNeighborAddress() {
    return folly::IPAddressV4("129.0.0.1");
  }

  template <typename AddrT = IPAddrT>
  std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV6>::value,
      folly::IPAddressV6> static getNeighborAddress() {
    return folly::IPAddressV6("2401::123");
  }
};

int getLookupClassFromL3Host(const bcm_l3_host_t& host) {
  return reinterpret_cast<const bcm_l3_host_t*>(&host)->l3a_lookup_class;
}

using PortNeighborV4 = NeighborT<folly::IPAddressV4, false>;
using TrunkNeighborV4 = NeighborT<folly::IPAddressV4, true>;
using PortNeighborV6 = NeighborT<folly::IPAddressV6, false>;
using TrunkNeighborV6 = NeighborT<folly::IPAddressV6, true>;

const facebook::fboss::AggregatePortID kAggID{1};

template <>
facebook::fboss::cfg::SwitchConfig PortNeighborV4::initialConfig(
    facebook::fboss::cfg::SwitchConfig config) {
  return config;
}

void setTrunk(facebook::fboss::cfg::SwitchConfig* config) {
  std::vector<int> ports;
  for (auto port : config->ports) {
    ports.push_back(port.logicalID);
  }
  facebook::fboss::utility::addAggPort(kAggID, ports, config);
}

template <>
facebook::fboss::cfg::SwitchConfig TrunkNeighborV4::initialConfig(
    facebook::fboss::cfg::SwitchConfig config) {
  setTrunk(&config);
  return config;
}

template <>
facebook::fboss::cfg::SwitchConfig PortNeighborV6::initialConfig(
    facebook::fboss::cfg::SwitchConfig config) {
  return config;
}

template <>
facebook::fboss::cfg::SwitchConfig TrunkNeighborV6::initialConfig(
    facebook::fboss::cfg::SwitchConfig config) {
  setTrunk(&config);
  return config;
}

using NeighborTypes = ::testing::
    Types<PortNeighborV4, TrunkNeighborV4, PortNeighborV6, TrunkNeighborV6>;

} // namespace
namespace facebook::fboss {

template <typename NeighborT>
class BcmNeighborTests : public BcmLinkStateDependentTests {
  using IPAddrT = typename NeighborT::IPAddrT;
  static auto constexpr programToTrunk = NeighborT::isTrunk;
  using NTable = typename std::conditional_t<
      std::is_same<IPAddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return NeighborT::initialConfig(utility::oneL3IntfTwoPortConfig(
        getHwSwitch(),
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
        cfg::PortLoopbackMode::MAC));
  }

  PortDescriptor portDescriptor() {
    if (programToTrunk)
      return PortDescriptor(kAggID);
    return PortDescriptor(masterLogicalPortIds()[0]);
  }

  std::shared_ptr<SwitchState> addNeighbor(
      const std::shared_ptr<SwitchState>& inState) {
    auto ip = NeighborT::getNeighborAddress();
    auto outState{inState->clone()};
    auto neighborTable = outState->getVlans()
                             ->getVlan(kVlanID)
                             ->template getNeighborTable<NTable>()
                             ->modify(kVlanID, &outState);
    neighborTable->addPendingEntry(ip, kIntfID);
    return outState;
  }

  std::shared_ptr<SwitchState> removeNeighbor(
      const std::shared_ptr<SwitchState>& inState) {
    auto ip = NeighborT::getNeighborAddress();
    auto outState{inState->clone()};

    auto neighborTable = outState->getVlans()
                             ->getVlan(kVlanID)
                             ->template getNeighborTable<NTable>()
                             ->modify(kVlanID, &outState);

    neighborTable->removeEntry(ip);
    return outState;
  }

  std::shared_ptr<SwitchState> resolveNeighbor(
      const std::shared_ptr<SwitchState>& inState) {
    auto ip = NeighborT::getNeighborAddress();
    auto outState{inState->clone()};
    auto neighborTable = outState->getVlans()
                             ->getVlan(kVlanID)
                             ->template getNeighborTable<NTable>()
                             ->modify(kVlanID, &outState);

    neighborTable->updateEntry(
        ip, kNeighborMac, portDescriptor(), kIntfID, kLookupClass);
    return outState;
  }

  std::shared_ptr<SwitchState> unresolveNeighbor(
      const std::shared_ptr<SwitchState>& inState) {
    return addNeighbor(removeNeighbor(inState));
  }

  BcmHost* getNeighborHost() {
    auto ip = NeighborT::getNeighborAddress();
    return this->getHwSwitch()->getNeighborTable()->getNeighborIf(
        BcmHostKey(0, ip, kIntfID));
  }

  bool isProgrammedToCPU(bcm_if_t egressId) {
    bcm_l3_egress_t egress;
    auto cpuFlags = (BCM_L3_L2TOCPU | BCM_L3_COPY_TO_CPU);
    bcm_l3_egress_t_init(&egress);
    bcm_l3_egress_get(0, egressId, &egress);
    return (egress.flags & cpuFlags) == cpuFlags;
  }

  template <typename AddrT = IPAddrT>
  void initHost(std::enable_if_t<
                std::is_same<AddrT, folly::IPAddressV4>::value,
                bcm_l3_host_t>* host) {
    auto ip = NeighborT::getNeighborAddress();
    bcm_l3_host_t_init(host);
    host->l3a_ip_addr = ip.toLongHBO();
  }

  template <typename AddrT = IPAddrT>
  void initHost(std::enable_if_t<
                std::is_same<AddrT, folly::IPAddressV6>::value,
                bcm_l3_host_t>* host) {
    auto ip = NeighborT::getNeighborAddress();
    bcm_l3_host_t_init(host);
    host->l3a_flags |= BCM_L3_IP6;
    ipToBcmIp6(ip, &(host->l3a_ip6_addr));
  }

  int getHost(bcm_l3_host_t* host) {
    initHost(host);
    return bcm_l3_host_find(0, host);
  }

  void verifyLookupClassL3(bcm_l3_host_t host, int classID) {
    /*
     * Queue-per-host classIDs are only supported for physical ports.
     * Pending entry should not have a classID (0) associated with it.
     * Resolved entry should have a classID associated with it.
     */
    EXPECT_TRUE(programToTrunk || getLookupClassFromL3Host(host) == classID);
  }

  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};

  const folly::MacAddress kNeighborMac{"1:2:3:4:5:6"};
  const cfg::AclLookupClass kLookupClass{
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2};
}; // namespace fboss

TYPED_TEST_CASE(BcmNeighborTests, NeighborTypes);

TYPED_TEST(BcmNeighborTests, AddPendingEntry) {
  auto setup = [this]() {
    auto newState = this->addNeighbor(this->getProgrammedState());
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    bcm_l3_host_t host;
    auto rv = this->getHost(&host);
    EXPECT_EQ(rv, 0);
    EXPECT_TRUE(this->isProgrammedToCPU(host.l3a_intf));
    this->verifyLookupClassL3(host, 0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(BcmNeighborTests, ResolvePendingEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    bcm_l3_host_t host;
    auto rv = this->getHost(&host);
    EXPECT_EQ(rv, 0);
    EXPECT_FALSE(this->isProgrammedToCPU(host.l3a_intf));
    this->verifyLookupClassL3(host, static_cast<int>(this->kLookupClass));
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(BcmNeighborTests, UnresolveResolvedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    auto newState = this->unresolveNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    bcm_l3_host_t host;
    auto rv = this->getHost(&host);
    EXPECT_EQ(rv, 0);
    EXPECT_TRUE(this->isProgrammedToCPU(host.l3a_intf));
    this->verifyLookupClassL3(host, 0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(BcmNeighborTests, ResolveThenUnresolveEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    this->applyNewState(state);
    auto newState = this->unresolveNeighbor(this->getProgrammedState());
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    bcm_l3_host_t host;
    auto rv = this->getHost(&host);
    EXPECT_EQ(rv, 0);
    EXPECT_TRUE(this->isProgrammedToCPU(host.l3a_intf));
    this->verifyLookupClassL3(host, 0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}
TYPED_TEST(BcmNeighborTests, RemoveResolvedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    auto newState = this->removeNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    bcm_l3_host_t host;
    auto rv = this->getHost(&host);
    EXPECT_EQ(rv, BCM_E_NOT_FOUND);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(BcmNeighborTests, AddPendingRemovedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    this->applyNewState(this->removeNeighbor(state));
    this->applyNewState(this->addNeighbor(this->getProgrammedState()));
  };
  auto verify = [this]() {
    bcm_l3_host_t host;
    auto rv = this->getHost(&host);
    EXPECT_EQ(rv, 0);
    EXPECT_TRUE(this->isProgrammedToCPU(host.l3a_intf));
    this->verifyLookupClassL3(host, 0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(BcmNeighborTests, LinkDownOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);
    this->bringDownPort(this->masterLogicalPortIds()[0]);
  };
  auto verify = [this]() {
    bcm_l3_host_t host;
    auto rv = this->getHost(&host);
    EXPECT_EQ(rv, 0);
    // egress to neighbor entry is not updated on link down
    // if it is not part of ecmp group
    EXPECT_FALSE(this->isProgrammedToCPU(host.l3a_intf));
    this->verifyLookupClassL3(host, static_cast<int>(this->kLookupClass));
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(BcmNeighborTests, BothLinkDownOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);

    this->bringDownPorts(
        {this->masterLogicalPortIds()[0], this->masterLogicalPortIds()[1]});
  };
  auto verify = [this]() {
    bcm_l3_host_t host;
    auto rv = this->getHost(&host);
    EXPECT_EQ(rv, 0);
    // egress to neighbor entry is not updated on link down
    // if it is not part of ecmp group
    EXPECT_FALSE(this->isProgrammedToCPU(host.l3a_intf));
    this->verifyLookupClassL3(host, static_cast<int>(this->kLookupClass));
  };

  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(BcmNeighborTests, LinkDownAndUpOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);
    this->bringDownPort(this->masterLogicalPortIds()[0]);
    this->bringUpPort(this->masterLogicalPortIds()[0]);
  };
  auto verifyForPort = [this]() {
    bcm_l3_host_t host;
    auto rv = this->getHost(&host);
    EXPECT_EQ(rv, 0);
    EXPECT_FALSE(this->isProgrammedToCPU(host.l3a_intf));
    this->verifyLookupClassL3(host, static_cast<int>(this->kLookupClass));
  };
  auto verifyForTrunk = [this]() {
    bcm_l3_host_t host;
    auto rv = this->getHost(&host);
    EXPECT_EQ(rv, 0);
    EXPECT_FALSE(this->isProgrammedToCPU(host.l3a_intf));
  };

  if (TypeParam::isTrunk) {
    setup();
    verifyForTrunk();
  } else {
    this->verifyAcrossWarmBoots(setup, verifyForPort);
  }
}

} // namespace facebook::fboss
