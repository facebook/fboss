// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/GtestDefs.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/test/HwNeighborTests.h"

using namespace ::testing;

extern "C" {
#include <bcm/l3.h>
}

namespace facebook::fboss {

template <typename NeighborT>
class BcmNeighborTest : public HwNeighborTest<NeighborT> {
 protected:
  using BaseT = HwNeighborTest<NeighborT>;
  using IPAddrT = typename BaseT::IPAddrT;

  BcmHost* getNeighborHost() {
    auto ip = NeighborT::getNeighborAddress();
    return this->getHwSwitch()->getNeighborTable()->getNeighborIf(
        BcmHostKey(0, ip, BaseT::kIntfID));
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
    EXPECT_TRUE(BaseT::programToTrunk || host.l3a_lookup_class == classID);
  }
};

TYPED_TEST_SUITE(BcmNeighborTest, NeighborTypes);

TYPED_TEST(BcmNeighborTest, AddPendingEntry) {
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

TYPED_TEST(BcmNeighborTest, ResolvePendingEntry) {
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

TYPED_TEST(BcmNeighborTest, UnresolveResolvedEntry) {
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

TYPED_TEST(BcmNeighborTest, ResolveThenUnresolveEntry) {
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
TYPED_TEST(BcmNeighborTest, RemoveResolvedEntry) {
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

TYPED_TEST(BcmNeighborTest, AddPendingRemovedEntry) {
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

TYPED_TEST(BcmNeighborTest, LinkDownOnResolvedEntry) {
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

TYPED_TEST(BcmNeighborTest, BothLinkDownOnResolvedEntry) {
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

TYPED_TEST(BcmNeighborTest, LinkDownAndUpOnResolvedEntry) {
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
