// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>

#include "fboss/agent/GtestDefs.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestNeighborUtils.h"
#include "fboss/agent/test/TrunkUtils.h"

using namespace ::testing;

namespace facebook::fboss {

template <typename AddrType, bool trunk = false>
struct NeighborT {
  using IPAddrT = AddrType;
  static constexpr auto isTrunk = trunk;
  static facebook::fboss::cfg::SwitchConfig initialConfig(
      facebook::fboss::cfg::SwitchConfig config);

  template <typename AddrT = IPAddrT>
  std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      folly::IPAddressV4> static getNeighborAddress() {
    return folly::IPAddressV4("1.1.1.2");
  }

  template <typename AddrT = IPAddrT>
  std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV6>::value,
      folly::IPAddressV6> static getNeighborAddress() {
    return folly::IPAddressV6("1::2");
  }
};

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
  for (auto port : *config->ports_ref()) {
    ports.push_back(*port.logicalID_ref());
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

template <typename NeighborT>
class HwNeighborTest : public HwLinkStateDependentTest {
 protected:
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

  void verifyClassId(int classID) {
    /*
     * Queue-per-host classIDs are only supported for physical ports.
     * Pending entry should not have a classID (0) associated with it.
     * Resolved entry should have a classID associated with it.
     */
    auto gotClassid = utility::getNbrClassId(
        this->getHwSwitch(), kIntfID, NeighborT::getNeighborAddress());
    EXPECT_TRUE(programToTrunk || classID == gotClassid.value());
  }
  folly::IPAddress getNeighborAddress() const {
    return NeighborT::getNeighborAddress();
  }
  bool isProgrammedToCPU() const {
    return utility::nbrProgrammedToCpu(
        this->getHwSwitch(), kIntfID, this->getNeighborAddress());
  }

  const VlanID kVlanID{utility::kBaseVlanId};
  const InterfaceID kIntfID{utility::kBaseVlanId};

  const folly::MacAddress kNeighborMac{"1:2:3:4:5:6"};
  const cfg::AclLookupClass kLookupClass{
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2};
};

TYPED_TEST_SUITE(HwNeighborTest, NeighborTypes);

TYPED_TEST(HwNeighborTest, AddPendingEntry) {
  auto setup = [this]() {
    auto newState = this->addNeighbor(this->getProgrammedState());
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    EXPECT_TRUE(this->isProgrammedToCPU());
    this->verifyClassId(0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, ResolvePendingEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    EXPECT_FALSE(this->isProgrammedToCPU());
    this->verifyClassId(static_cast<int>(this->kLookupClass));
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, UnresolveResolvedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    auto newState = this->unresolveNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    EXPECT_TRUE(this->isProgrammedToCPU());
    this->verifyClassId(0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, ResolveThenUnresolveEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    this->applyNewState(state);
    auto newState = this->unresolveNeighbor(this->getProgrammedState());
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    EXPECT_TRUE(this->isProgrammedToCPU());
    this->verifyClassId(0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}
TYPED_TEST(HwNeighborTest, RemoveResolvedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    auto newState = this->removeNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() { EXPECT_ANY_THROW(this->isProgrammedToCPU()); };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, AddPendingRemovedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    this->applyNewState(this->removeNeighbor(state));
    this->applyNewState(this->addNeighbor(this->getProgrammedState()));
  };
  auto verify = [this]() {
    EXPECT_TRUE(this->isProgrammedToCPU());
    this->verifyClassId(0);
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, LinkDownOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);
    this->bringDownPort(this->masterLogicalPortIds()[0]);
  };
  auto verify = [this]() {
    // egress to neighbor entry is not updated on link down
    // if it is not part of ecmp group
    EXPECT_FALSE(this->isProgrammedToCPU());
    this->verifyClassId(static_cast<int>(this->kLookupClass));
  };
  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, BothLinkDownOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);

    this->bringDownPorts(
        {this->masterLogicalPortIds()[0], this->masterLogicalPortIds()[1]});
  };
  auto verify = [this]() {
    // egress to neighbor entry is not updated on link down
    // if it is not part of ecmp group
    EXPECT_FALSE(this->isProgrammedToCPU());
    this->verifyClassId(static_cast<int>(this->kLookupClass));
  };

  if (TypeParam::isTrunk) {
    setup();
    verify();
  } else {
    this->verifyAcrossWarmBoots(setup, verify);
  }
}

TYPED_TEST(HwNeighborTest, LinkDownAndUpOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);
    this->bringDownPort(this->masterLogicalPortIds()[0]);
    this->bringUpPort(this->masterLogicalPortIds()[0]);
  };
  auto verifyForPort = [this]() {
    EXPECT_FALSE(this->isProgrammedToCPU());
    this->verifyClassId(static_cast<int>(this->kLookupClass));
  };
  auto verifyForTrunk = [this]() { EXPECT_FALSE(this->isProgrammedToCPU()); };

  if (TypeParam::isTrunk) {
    setup();
    verifyForTrunk();
  } else {
    this->verifyAcrossWarmBoots(setup, verifyForPort);
  }
}

} // namespace facebook::fboss
