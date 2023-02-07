// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>
#include <algorithm>

#include "fboss/agent/GtestDefs.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestNeighborUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/TrunkUtils.h"

using namespace ::testing;

namespace facebook::fboss {

template <typename AddrType, bool trunk = false>
struct NeighborT {
  using IPAddrT = AddrType;
  static constexpr auto isTrunk = trunk;

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
    auto cfg = programToTrunk ? utility::oneL3IntfTwoPortConfig(
                                    getHwSwitch(),
                                    masterLogicalPortIds()[0],
                                    masterLogicalPortIds()[1],
                                    getAsic()->desiredLoopbackMode())
                              : utility::onePortPerInterfaceConfig(
                                    getHwSwitch(),
                                    masterLogicalPortIds(),
                                    getAsic()->desiredLoopbackMode());
    if (programToTrunk) {
      // Keep member size to be less than/equal to HW limitation, but first add
      // the two ports for testing.
      std::set<int> portSet{
          masterLogicalPortIds()[0], masterLogicalPortIds()[1]};
      int idx = 0;
      while (portSet.size() <
             std::min(
                 getAsic()->getMaxLagMemberSize(),
                 static_cast<uint32_t>((*cfg.ports()).size()))) {
        portSet.insert(*cfg.ports()[idx].logicalID());
        idx++;
      }
      std::vector<int> ports(portSet.begin(), portSet.end());
      facebook::fboss::utility::addAggPort(kAggID, ports, &cfg);
    }
    return cfg;
  }
  VlanID kVlanID() const {
    if (getProgrammedState()->getSwitchSettings()->getSwitchType() ==
        cfg::SwitchType::NPU) {
      auto vlanId = utility::firstVlanID(getProgrammedState());
      CHECK(vlanId.has_value());
      return *vlanId;
    }
    XLOG(FATAL) << " No vlans on non-npu switches";
  }
  InterfaceID kIntfID() const {
    if (getProgrammedState()->getSwitchSettings()->getSwitchType() ==
        cfg::SwitchType::NPU) {
      return InterfaceID(static_cast<int>(kVlanID()));
    }
    XLOG(FATAL) << "TODO: VOQ switch support";
  }

  PortDescriptor portDescriptor() {
    if (programToTrunk) {
      return PortDescriptor(kAggID);
    }
    return PortDescriptor(masterLogicalPortIds()[0]);
  }

  auto getNeighborTable(std::shared_ptr<SwitchState> state) {
    if (state->getSwitchSettings()->getSwitchType() == cfg::SwitchType::NPU) {
      return state->getVlans()
          ->getVlan(kVlanID())
          ->template getNeighborTable<NTable>()
          ->modify(kVlanID(), &state);
    }
    XLOG(FATAL) << "TODO: VOQ switch support";
  }
  std::shared_ptr<SwitchState> addNeighbor(
      const std::shared_ptr<SwitchState>& inState) {
    auto ip = NeighborT::getNeighborAddress();
    auto outState{inState->clone()};

    auto neighborTable = getNeighborTable(outState);
    neighborTable->addPendingEntry(ip, kIntfID());
    return outState;
  }

  std::shared_ptr<SwitchState> removeNeighbor(
      const std::shared_ptr<SwitchState>& inState) {
    auto ip = NeighborT::getNeighborAddress();
    auto outState{inState->clone()};

    auto neighborTable = getNeighborTable(outState);
    neighborTable->removeEntry(ip);
    return outState;
  }

  std::shared_ptr<SwitchState> resolveNeighbor(
      const std::shared_ptr<SwitchState>& inState,
      std::optional<cfg::AclLookupClass> lookupClass = std::nullopt) {
    auto ip = NeighborT::getNeighborAddress();
    auto outState{inState->clone()};
    auto neighborTable = getNeighborTable(outState);
    auto lookupClassValue = lookupClass ? lookupClass.value() : kLookupClass;
    neighborTable->updateEntry(
        ip,
        kNeighborMac,
        portDescriptor(),
        kIntfID(),
        NeighborState::REACHABLE,
        lookupClassValue);
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
        this->getHwSwitch(), kIntfID(), NeighborT::getNeighborAddress());
    EXPECT_TRUE(programToTrunk || classID == gotClassid.value());
  }

  bool nbrExists() const {
    return utility::nbrExists(
        this->getHwSwitch(), this->kIntfID(), this->getNeighborAddress());
  }
  folly::IPAddress getNeighborAddress() const {
    return NeighborT::getNeighborAddress();
  }

  bool isProgrammedToCPU() const {
    return utility::nbrProgrammedToCpu(
        this->getHwSwitch(), kIntfID(), this->getNeighborAddress());
  }

  const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
  const cfg::AclLookupClass kLookupClass{
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2};
  const cfg::AclLookupClass kLookupClass2{
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3};
};

TYPED_TEST_SUITE(HwNeighborTest, NeighborTypes);

TYPED_TEST(HwNeighborTest, AddPendingEntry) {
  auto setup = [this]() {
    auto newState = this->addNeighbor(this->getProgrammedState());
    this->applyNewState(newState);
  };
  auto verify = [this]() { EXPECT_TRUE(this->isProgrammedToCPU()); };
  this->verifyAcrossWarmBoots(setup, verify);
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
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwNeighborTest, ResolvePendingEntryThenChangeLookupClass) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    this->applyNewState(newState);
    newState = this->resolveNeighbor(state, this->kLookupClass2);
    this->applyNewState(newState);
  };
  auto verify = [this]() {
    EXPECT_FALSE(this->isProgrammedToCPU());
    this->verifyClassId(static_cast<int>(this->kLookupClass2));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwNeighborTest, UnresolveResolvedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    auto newState = this->unresolveNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() { EXPECT_TRUE(this->isProgrammedToCPU()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwNeighborTest, ResolveThenUnresolveEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    this->applyNewState(state);
    auto newState = this->unresolveNeighbor(this->getProgrammedState());
    this->applyNewState(newState);
  };
  auto verify = [this]() { EXPECT_TRUE(this->isProgrammedToCPU()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwNeighborTest, RemoveResolvedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    auto newState = this->removeNeighbor(state);
    this->applyNewState(newState);
  };
  auto verify = [this]() { EXPECT_FALSE(this->nbrExists()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwNeighborTest, AddPendingRemovedEntry) {
  auto setup = [this]() {
    auto state =
        this->resolveNeighbor(this->addNeighbor(this->getProgrammedState()));
    this->applyNewState(this->removeNeighbor(state));
    this->applyNewState(this->addNeighbor(this->getProgrammedState()));
  };
  auto verify = [this]() { EXPECT_TRUE(this->isProgrammedToCPU()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwNeighborTest, LinkDownOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);
    this->bringDownPort(this->masterLogicalPortIds()[0]);
  };
  auto verify = [this]() {
    // There is a behavior differnce b/w SAI and BcmSwitch on link down
    // w.r.t. to neighbor entries. In BcmSwitch, we leave the nbr entries
    // intact and rely on SwSwitch to prune them. In SaiSwitch we prune
    // the fdb and neighbor entries on in SaiSwitch itself
    if (this->nbrExists()) {
      // egress to neighbor entry is not updated on link down
      // if it is not part of ecmp group
      EXPECT_FALSE(this->isProgrammedToCPU());
      this->verifyClassId(static_cast<int>(this->kLookupClass));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwNeighborTest, LinkDownAndUpOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);
    this->bringDownPort(this->masterLogicalPortIds()[0]);
    this->bringUpPort(this->masterLogicalPortIds()[0]);
  };
  auto verify = [this]() {
    // There is a behavior differnce b/w SAI and BcmSwitch on link down
    // w.r.t. to neighbor entries. In BcmSwitch, we leave the nbr entries
    // intact and rely on SwSwitch to prune them. In SaiSwitch we prune
    // the fdb and neighbor entries on in SaiSwitch itself
    if (this->nbrExists()) {
      // egress to neighbor entry is not updated on link down
      // if it is not part of ecmp group
      EXPECT_FALSE(this->isProgrammedToCPU());
      this->verifyClassId(static_cast<int>(this->kLookupClass));
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
