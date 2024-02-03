// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>
#include <algorithm>

#include "fboss/agent/GtestDefs.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestNeighborUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"

/*
 * Adding a new flag which can be used to setup ports in non-loopback
 * mode. Some vendors have HwTests from OSS running in their environment
 * against simulator. This new option will provide a control plane
 * (from the test binary) to program forwarding plane (setup neighbors,
 * bringup ports etc.) of simulator. Once the dataplane is programmed,
 * network namespaces could be used to setup an environment to inject
 * traffic to the simulator. This can be used for throughput test of
 * ASIC simulator in the same env vendors use for regular HwTests.
 */
DEFINE_bool(disable_loopback, false, "Disable loopback on test ports");

DECLARE_bool(intf_nbr_tables);

using namespace ::testing;

namespace facebook::fboss {

template <typename AddrType, bool trunk = false, bool intfNbrTable = false>
struct NeighborT {
  using IPAddrT = AddrType;
  static constexpr auto isTrunk = trunk;
  static auto constexpr isIntfNbrTable = intfNbrTable;

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

using PortNeighborV4VlanNbrTable = NeighborT<folly::IPAddressV4, false, false>;
using TrunkNeighborV4VlanNbrTable = NeighborT<folly::IPAddressV4, true, false>;
using PortNeighborV6VlanNbrTable = NeighborT<folly::IPAddressV6, false, false>;
using TrunkNeighborV6VlanNbrTable = NeighborT<folly::IPAddressV6, true, false>;

using PortNeighborV4IntfNbrTable = NeighborT<folly::IPAddressV4, false, true>;
using TrunkNeighborV4IntfNbrTable = NeighborT<folly::IPAddressV4, true, true>;
using PortNeighborV6IntfNbrTable = NeighborT<folly::IPAddressV6, false, true>;
using TrunkNeighborV6IntfNbrTable = NeighborT<folly::IPAddressV6, true, true>;

const facebook::fboss::AggregatePortID kAggID{1};

using NeighborTypes = ::testing::Types<
    PortNeighborV4VlanNbrTable,
    TrunkNeighborV4VlanNbrTable,
    PortNeighborV6VlanNbrTable,
    TrunkNeighborV6VlanNbrTable,
    PortNeighborV4IntfNbrTable,
    TrunkNeighborV4IntfNbrTable,
    PortNeighborV6IntfNbrTable,
    TrunkNeighborV6IntfNbrTable>;

template <typename NeighborT>
class HwNeighborTest : public HwLinkStateDependentTest {
 protected:
  using IPAddrT = typename NeighborT::IPAddrT;
  static auto constexpr programToTrunk = NeighborT::isTrunk;
  static auto constexpr isIntfNbrTable = NeighborT::isIntfNbrTable;
  using NTable = typename std::conditional_t<
      std::is_same<IPAddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

 protected:
  void SetUp() override {
    FLAGS_intf_nbr_tables = isIntfNbrTable;
    HwLinkStateDependentTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = programToTrunk
        ? utility::oneL3IntfTwoPortConfig(
              getHwSwitch()->getPlatform()->getPlatformMapping(),
              getHwSwitch()->getPlatform()->getAsic(),
              masterLogicalPortIds()[0],
              masterLogicalPortIds()[1],
              getHwSwitch()->getPlatform()->supportsAddRemovePort(),
              getAsic()->desiredLoopbackModes())
        : utility::onePortPerInterfaceConfig(
              getHwSwitch(),
              masterLogicalPortIds(),
              getAsic()->desiredLoopbackModes());
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
    if (getSwitchType() == cfg::SwitchType::NPU) {
      auto vlanId = utility::firstVlanID(getProgrammedState());
      CHECK(vlanId.has_value());
      return *vlanId;
    }
    XLOG(FATAL) << " No vlans on non-npu switches";
  }
  InterfaceID kIntfID() const {
    auto switchType = getSwitchType();
    if (switchType == cfg::SwitchType::NPU) {
      return InterfaceID(static_cast<int>(kVlanID()));
    } else if (switchType == cfg::SwitchType::VOQ) {
      CHECK(!programToTrunk) << " Trunks not supported yet on VOQ switches";
      auto portId = this->portDescriptor().phyPortID();
      return InterfaceID((*getProgrammedState()
                               ->getPorts()
                               ->getNodeIf(portId)
                               ->getInterfaceIDs()
                               .begin()));
    }
    XLOG(FATAL) << "Unexpected switch type " << static_cast<int>(switchType);
  }

  PortDescriptor portDescriptor() const {
    if (programToTrunk) {
      return PortDescriptor(kAggID);
    }
    return PortDescriptor(masterLogicalInterfacePortIds()[0]);
  }

  auto getNeighborTable(std::shared_ptr<SwitchState> state) {
    auto switchType = getSwitchType();

    if (isIntfNbrTable || switchType == cfg::SwitchType::VOQ) {
      return state->getInterfaces()
          ->getNode(kIntfID())
          ->template getNeighborEntryTable<IPAddrT>()
          ->modify(kIntfID(), &state);
    } else if (switchType == cfg::SwitchType::NPU) {
      return state->getVlans()
          ->getNode(kVlanID())
          ->template getNeighborTable<NTable>()
          ->modify(kVlanID(), &state);
    }

    XLOG(FATAL) << "Unexpected switch type " << static_cast<int>(switchType);
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
    neighborTable->updateEntry(
        ip,
        kNeighborMac,
        portDescriptor(),
        kIntfID(),
        NeighborState::REACHABLE,
        lookupClass);
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
    XLOG(INFO) << " GOT CLASSID: " << gotClassid.value();
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
  auto verify = [this]() { EXPECT_FALSE(this->isProgrammedToCPU()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwNeighborTest, ResolvePendingEntryThenChangeLookupClass) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state, this->kLookupClass);
    this->applyNewState(newState);
    this->verifyClassId(static_cast<int>(this->kLookupClass));
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
    this->bringDownPort(this->masterLogicalInterfacePortIds()[0]);
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
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwNeighborTest, LinkDownAndUpOnResolvedEntry) {
  auto setup = [this]() {
    auto state = this->addNeighbor(this->getProgrammedState());
    auto newState = this->resolveNeighbor(state);
    newState = this->applyNewState(newState);
    this->bringDownPort(this->masterLogicalInterfacePortIds()[0]);
    this->bringUpPort(this->masterLogicalInterfacePortIds()[0]);
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
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

template <bool enableIntfNbrTable>
struct EnableIntfNbrTable {
  static constexpr auto intfNbrTable = enableIntfNbrTable;
};

using IntfNbrTableTypes =
    ::testing::Types<EnableIntfNbrTable<false>, EnableIntfNbrTable<true>>;

template <typename EnableIntfNbrTableT>
class HwNeighborOnMultiplePortsTest : public HwLinkStateDependentTest {
  static auto constexpr isIntfNbrTable = EnableIntfNbrTableT::intfNbrTable;

 protected:
  void SetUp() override {
    FLAGS_intf_nbr_tables = isIntfNbrTable;
    HwLinkStateDependentTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
  }

  void oneNeighborPerPortSetup(const std::vector<PortID>& portIds) {
    auto cfg = initialConfig();
    if (FLAGS_disable_loopback) {
      // Disable loopback on specific ports
      for (auto portId : portIds) {
        auto portCfg = utility::findCfgPortIf(cfg, portId);
        if (portCfg != cfg.ports()->end()) {
          portCfg->loopbackMode() = cfg::PortLoopbackMode::NONE;
        }
      }
      applyNewConfig(cfg);
    }

    // Create adjacencies on all test ports
    auto dstMac = utility::getFirstInterfaceMac(cfg);
    for (int idx = 0; idx < portIds.size(); idx++) {
      utility::EcmpSetupAnyNPorts6 ecmpHelper6(
          getProgrammedState(),
          utility::MacAddressGenerator().get(dstMac.u64NBO() + idx + 1));
      applyNewState(ecmpHelper6.resolveNextHops(
          getProgrammedState(), {PortDescriptor(portIds[idx])}));
    }

    // Dump the local interface config
    XLOG(DBG0) << "Dumping port configurations:";
    for (int idx = 0; idx < portIds.size(); idx++) {
      auto mac = utility::getFirstInterfaceMac(getProgrammedState());
      XLOG(DBG0) << "   Port " << portIds[idx]
                 << ", IPv6: " << cfg.interfaces()[idx].ipAddresses()[1]
                 << ", Intf MAC: " << mac;
    }
  }

  InterfaceID getInterfaceId(const PortID& portId) const {
    auto switchType = getSwitchType();

    if (isIntfNbrTable || switchType == cfg::SwitchType::VOQ) {
      return InterfaceID(*getProgrammedState()
                              ->getPorts()
                              ->getNodeIf(portId)
                              ->getInterfaceIDs()
                              .begin());
    } else if (switchType == cfg::SwitchType::NPU) {
      return InterfaceID(static_cast<int>((*getProgrammedState()
                                                ->getPorts()
                                                ->getNodeIf(portId)
                                                ->getVlans()
                                                .begin())
                                              .first));
    }

    XLOG(FATAL) << "Unexpected switch type " << static_cast<int>(switchType);
  }

  bool isProgrammedToCPU(const PortID& portId, const folly::IPAddress& ip)
      const {
    auto intfId = getInterfaceId(portId);
    return utility::nbrProgrammedToCpu(this->getHwSwitch(), intfId, ip);
  }
};

TYPED_TEST_SUITE(HwNeighborOnMultiplePortsTest, IntfNbrTableTypes);

TYPED_TEST(HwNeighborOnMultiplePortsTest, ResolveOnTwoPorts) {
  auto setup = [&]() {
    this->oneNeighborPerPortSetup(
        {this->masterLogicalInterfacePortIds()[0],
         this->masterLogicalInterfacePortIds()[1]});
  };
  auto verify = [&]() {
    EXPECT_FALSE(this->isProgrammedToCPU(
        this->masterLogicalInterfacePortIds()[0], folly::IPAddressV6("1::1")));
    EXPECT_FALSE(this->isProgrammedToCPU(
        this->masterLogicalInterfacePortIds()[1], folly::IPAddressV6("2::2")));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
