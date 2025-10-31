// Copyright 2004-present Facebook. All Rights Reserved.

#include <gtest/gtest.h>
#include <algorithm>

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/NeighborTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"

#include "fboss/lib/CommonUtils.h"

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

namespace {
facebook::fboss::utility::NeighborInfo getNeighborInfo(
    const facebook::fboss::InterfaceID& interfaceId,
    const folly::IPAddress& ip,
    facebook::fboss::AgentEnsemble& ensemble) {
  auto switchId = ensemble.getSw()
                      ->getScopeResolver()
                      ->scope(ensemble.masterLogicalPortIds())
                      .switchId();
  auto client = ensemble.getHwAgentTestClient(switchId);
  facebook::fboss::IfAndIP neighbor;
  neighbor.interfaceID() = interfaceId;
  neighbor.ip() = facebook::network::toBinaryAddress(ip);
  facebook::fboss::utility::NeighborInfo neighborInfo;
  client->sync_getNeighborInfo(neighborInfo, std::move(neighbor));
  return neighborInfo;
}
} // namespace

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
    return folly::IPAddressV4("1.0.0.2");
  }
  template <typename AddrT = IPAddrT>
  std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV4>::value,
      folly::IPAddressV4> static getLinkLocalNeighborAddress() {
    return folly::IPAddressV4("192.168.0.1");
  }

  template <typename AddrT = IPAddrT>
  std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV6>::value,
      folly::IPAddressV6> static getNeighborAddress() {
    return folly::IPAddressV6("1::2");
  }
  template <typename AddrT = IPAddrT>
  std::enable_if_t<
      std::is_same<AddrT, folly::IPAddressV6>::value,
      folly::IPAddressV6> static getLinkLocalNeighborAddress() {
    return folly::IPAddressV6("fe80::2ae7:1dff:fe73:f21a");
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
class AgentNeighborTest : public AgentHwTest {
 protected:
  using IPAddrT = typename NeighborT::IPAddrT;
  static auto constexpr programToTrunk = NeighborT::isTrunk;
  static auto constexpr isIntfNbrTable = NeighborT::isIntfNbrTable;
  using NTable = typename std::conditional_t<
      std::is_same<IPAddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

 protected:
  void setCmdLineFlagOverrides() const override {
    FLAGS_intf_nbr_tables = isIntfNbrTable;
    AgentHwTest::setCmdLineFlagOverrides();
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    std::vector<ProductionFeature> features = {
        ProductionFeature::L3_FORWARDING};
    if (isIntfNbrTable) {
      features.push_back(ProductionFeature::INTERFACE_NEIGHBOR_TABLE);
    } else {
      features.push_back(ProductionFeature::VLAN);
    }
    if (programToTrunk) {
      features.push_back(ProductionFeature::LAG);
    }
    return features;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = programToTrunk
        ? utility::oneL3IntfTwoPortConfig(
              ensemble.getSw(),
              ensemble.masterLogicalPortIds()[0],
              ensemble.masterLogicalPortIds()[1])
        : utility::onePortPerInterfaceConfig(
              ensemble.getSw(), ensemble.masterLogicalPortIds());
    if (programToTrunk) {
      // Keep member size to be less than/equal to HW limitation, but first add
      // the two ports for testing.
      std::set<int> portSet{
          ensemble.masterLogicalPortIds()[0],
          ensemble.masterLogicalPortIds()[1]};
      int idx = 0;
      while (
          portSet.size() <
          std::min(
              checkSameAndGetAsic(ensemble.getL3Asics())->getMaxLagMemberSize(),
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
    if (checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())
            ->getSwitchType() == cfg::SwitchType::NPU) {
      auto vlanId = getVlanIDForTx();
      CHECK(vlanId.has_value());
      return *vlanId;
    }
    XLOG(FATAL) << " No vlans on non-npu switches";
  }
  InterfaceID kIntfID() const {
    auto switchType =
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getSwitchType();
    if (switchType == cfg::SwitchType::NPU) {
      if (!isIntfNbrTable) {
        return InterfaceID(static_cast<int>(kVlanID()));
      } else {
        return utility::firstInterfaceIDWithPorts(getProgrammedState());
      }
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
    auto switchType =
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getSwitchType();

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
      const std::shared_ptr<SwitchState>& inState,
      bool linkLocal = false) {
    auto ip = getNeighborAddress(linkLocal);
    auto outState{inState->clone()};
    auto neighborTable = getNeighborTable(outState);
    neighborTable->addPendingEntry(ip, kIntfID());
    return outState;
  }

  std::shared_ptr<SwitchState> removeNeighbor(
      const std::shared_ptr<SwitchState>& inState,
      bool linkLocal = false) {
    auto ip = getNeighborAddress(linkLocal);
    auto outState{inState->clone()};

    auto neighborTable = getNeighborTable(outState);
    auto intf = outState->getInterfaces()->getNode(kIntfID());
    if (getSw()->needL2EntryForNeighbor() &&
        intf->getType() == cfg::InterfaceType::VLAN) {
      CHECK(intf->getVlanIDIf().has_value());
      outState = utility::NeighborTestUtils::pruneMacEntryForDelNbrEntry(
          outState, intf->getVlanID(), neighborTable->getEntryIf(ip));
    }
    neighborTable->removeEntry(ip);
    return outState;
  }

  std::shared_ptr<SwitchState> resolveNeighbor(
      const std::shared_ptr<SwitchState>& inState,
      std::optional<cfg::AclLookupClass> lookupClass = std::nullopt,
      bool linkLocal = false) {
    auto ip = getNeighborAddress(linkLocal);
    auto outState{inState->clone()};
    auto neighborTable = getNeighborTable(outState);
    auto switchType =
        checkSameAndGetAsic(this->getAgentEnsemble()->getL3Asics())
            ->getSwitchType();
    PortDescriptor port = portDescriptor();
    SystemPortID systemPortID;
    // VOQ switches stores system port ID in switch state
    if (switchType == cfg::SwitchType::VOQ) {
      systemPortID = SystemPortID(kIntfID());
      port = PortDescriptor(SystemPortID(systemPortID));
    }

    neighborTable->updateEntry(
        ip,
        kNeighborMac,
        port,
        kIntfID(),
        NeighborState::REACHABLE,
        lookupClass);

    auto intf = outState->getInterfaces()->getNode(kIntfID());
    if (getSw()->needL2EntryForNeighbor() &&
        intf->getType() == cfg::InterfaceType::VLAN) {
      CHECK(intf->getVlanIDIf().has_value());
      outState = utility::NeighborTestUtils::addMacEntryForNewNbrEntry(
          outState, intf->getVlanID(), neighborTable->getEntryIf(ip));
    }
    return outState;
  }

  std::shared_ptr<SwitchState> unresolveNeighbor(
      const std::shared_ptr<SwitchState>& inState,
      bool linkLocal = false) {
    return addNeighbor(removeNeighbor(inState, linkLocal), linkLocal);
  }

  void verifyClassId(int classID, bool linkLocal = false) {
    /*
     * Queue-per-host classIDs are only supported for physical ports.
     * Pending entry should not have a classID (0) associated with it.
     * Resolved entry should have a classID associated with it.
     */
    WITH_RETRIES({
      auto neighborInfo = getNeighborInfo(
          kIntfID(), getNeighborAddress(linkLocal), *getAgentEnsemble());
      EXPECT_EVENTUALLY_TRUE(neighborInfo.classId().has_value());
      if (neighborInfo.classId().has_value()) {
        EXPECT_EVENTUALLY_TRUE(
            programToTrunk || classID == neighborInfo.classId().value());
      }
    });
  }

  bool nbrExists(bool linkLocal = false) {
    auto neighborInfo = getNeighborInfo(
        kIntfID(), getNeighborAddress(linkLocal), *getAgentEnsemble());
    return *neighborInfo.exists();
  }
  IPAddrT getNeighborAddress(bool linkLocal) const {
    return linkLocal ? NeighborT::getLinkLocalNeighborAddress()
                     : NeighborT::getNeighborAddress();
  }

  bool isProgrammedToCPU(bool linkLocal = false) {
    auto neighborInfo = getNeighborInfo(
        kIntfID(), getNeighborAddress(linkLocal), *getAgentEnsemble());
    return *neighborInfo.isProgrammedToCpu();
  }

  const folly::MacAddress kNeighborMac{"2:3:4:5:6:7"};
  const cfg::AclLookupClass kLookupClass{
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2};
  const cfg::AclLookupClass kLookupClass2{
      cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3};
};

template <typename NeighborT>
class AgentNeighborResolutionTest : public AgentNeighborTest<NeighborT> {
 protected:
  using IPAddrT = typename NeighborT::IPAddrT;
  static auto constexpr isIntfNbrTable = NeighborT::isIntfNbrTable;
  using NTable = typename std::conditional_t<
      std::is_same<IPAddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

  void setCmdLineFlagOverrides() const override {
    FLAGS_intf_nbr_tables = isIntfNbrTable;
    AgentHwTest::setCmdLineFlagOverrides();
    // Enable neighbor cache so that class id is set
    FLAGS_disable_neighbor_updates = false;
    // enable neighbor update failure protection
    FLAGS_enable_hw_update_protection = true;
  }

  // Get the neighbor cache for the given address family
  template <typename AddrT = IPAddrT>
  auto getNeighborCache(std::shared_ptr<SwitchState> state) {
    auto switchType =
        checkSameAndGetAsic(this->getAgentEnsemble()->getL3Asics())
            ->getSwitchType();

    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      if (isIntfNbrTable || switchType == cfg::SwitchType::VOQ) {
        return this->getSw()
            ->getNeighborUpdater()
            ->getNdpCacheDataForIntf()
            .get();
      } else if (switchType == cfg::SwitchType::NPU) {
        return this->getSw()->getNeighborUpdater()->getNdpCacheData().get();
      } else {
        XLOG(FATAL) << "Unexpected switch type "
                    << static_cast<int>(switchType);
      }
    } else if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      if (isIntfNbrTable || switchType == cfg::SwitchType::VOQ) {
        return this->getSw()
            ->getNeighborUpdater()
            ->getArpCacheDataForIntf()
            .get();
      } else if (switchType == cfg::SwitchType::NPU) {
        return this->getSw()->getNeighborUpdater()->getArpCacheData().get();
      } else {
        XLOG(FATAL) << "Unexpected switch type "
                    << static_cast<int>(switchType);
      }
    } else {
      XLOG(FATAL) << "Unsupported address type";
    }
  }

  // Verify that neighbor entries are programmed in the neighbor cache and
  // switchState neighbor table has the correct port
  void verifyNeighborCache(
      const PortDescriptor& port,
      const std::shared_ptr<SwitchState>& inState) {
    auto outState{inState->clone()};
    auto ipAddress = this->getNeighborAddress(false);
    auto neighborTable = this->getNeighborTable(outState);
    auto switchType =
        checkSameAndGetAsic(this->getAgentEnsemble()->getL3Asics())
            ->getSwitchType();

    WITH_RETRIES({
      auto neighborCache = getNeighborCache(outState);

      auto cacheEntry = std::find_if(
          neighborCache.begin(),
          neighborCache.end(),
          [&ipAddress](const auto& entry) {
            return entry.ip() == network::toBinaryAddress(ipAddress);
          });
      EXPECT_EVENTUALLY_TRUE(cacheEntry != neighborCache.end());
      auto nbrEntry = neighborTable->getEntryIf(ipAddress);
      EXPECT_EVENTUALLY_TRUE(nbrEntry != nullptr);

      if (nbrEntry) {
        XLOG(DBG2) << "NDP : " << ipAddress.str() << " " << nbrEntry->str()
                   << " port " << nbrEntry->getPort().intID() << " port type "
                   << (int)nbrEntry->getPort().type();
        XLOG(DBG2) << "Neighbor port in cache: " << cacheEntry->port().value()
                   << " Port in switchState: " << port.intID();
        if (switchType == cfg::SwitchType::VOQ) {
          // VOQ switches store physical port ID in neighbor cache
          EXPECT_EVENTUALLY_EQ(cacheEntry->port().value(), port.intID());
          // VOQ switches store system port ID in switch state which is same as
          // interface ID
          EXPECT_EVENTUALLY_EQ(
              nbrEntry->getPort().intID(), cacheEntry->interfaceID().value());
        } else {
          EXPECT_EVENTUALLY_EQ(
              cacheEntry->port().value(), nbrEntry->getPort().intID());
        }
      }
    });
  }

  // Populate the neighbor cache from the switchState
  template <typename AddrT = IPAddrT>
  void populateNeighborsCache(const PortDescriptor& port) {
    auto interface =
        this->getProgrammedState()->getInterfaces()->getNodeIf(this->kIntfID());
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      this->populateNdpNeighborsToCache(interface);
    } else if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      this->populateArpNeighborsToCache(interface);
    }
  }
};

TYPED_TEST_SUITE(AgentNeighborTest, NeighborTypes);

TYPED_TEST(AgentNeighborTest, AddPendingEntry) {
  auto setup = [this]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return this->addNeighbor(in);
    });
  };
  auto verify = [this]() { EXPECT_TRUE(this->isProgrammedToCPU()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentNeighborTest, ResolvePendingEntry) {
  auto setup = [this]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto state = this->addNeighbor(in);
      return this->resolveNeighbor(state);
    });
  };
  auto verify = [this]() { EXPECT_FALSE(this->isProgrammedToCPU()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentNeighborTest, ResolveLinkLocalEntry) {
  bool linkLocal = true;
  auto setup = [this, linkLocal]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto state = this->addNeighbor(in, linkLocal);
      return this->resolveNeighbor(state, std::nullopt, linkLocal);
    });
  };
  // LL neighbors are not programmed in HW. So
  // there is nothing to verify. However we must
  // be able to come up and WB with LL neighbors
  // present
  this->verifyAcrossWarmBoots(setup, []() {});
}

TYPED_TEST(AgentNeighborTest, UnresolveResolvedEntry) {
  auto setup = [this]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto state = this->resolveNeighbor(this->addNeighbor(in));
      return this->unresolveNeighbor(state);
    });
  };
  auto verify = [this]() { EXPECT_TRUE(this->isProgrammedToCPU()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentNeighborTest, ResolveThenUnresolveEntry) {
  auto setup = [this]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return this->resolveNeighbor(this->addNeighbor(in));
    });
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return this->unresolveNeighbor(in);
    });
  };
  auto verify = [this]() { EXPECT_TRUE(this->isProgrammedToCPU()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentNeighborTest, RemoveResolvedEntry) {
  auto setup = [this]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto state = this->resolveNeighbor(this->addNeighbor(in));
      return this->removeNeighbor(state);
    });
  };
  auto verify = [this]() { EXPECT_FALSE(this->nbrExists()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentNeighborTest, AddPendingRemovedEntry) {
  auto setup = [this]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto state = this->resolveNeighbor(this->addNeighbor(in));
      return this->removeNeighbor(state);
    });
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return this->addNeighbor(in);
    });
  };
  auto verify = [this]() { EXPECT_TRUE(this->isProgrammedToCPU()); };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentNeighborTest, LinkDownOnResolvedEntry) {
  auto setup = [this]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto state = this->addNeighbor(in);
      return this->resolveNeighbor(state);
    });
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

TYPED_TEST(AgentNeighborTest, LinkDownAndUpOnResolvedEntry) {
  auto setup = [this]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto state = this->addNeighbor(in);
      return this->resolveNeighbor(state);
    });
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

TYPED_TEST_SUITE(AgentNeighborResolutionTest, NeighborTypes);
// This test verifies that the neighbor cache is populated correctly
// from the switchState. This is done by adding a neighbor entry
// and then populating the cache. The cache is then verified to
// have the correct port associated with the neighbor entry
TYPED_TEST(AgentNeighborResolutionTest, ResolveNeighborAndCheckCache) {
  auto setup = [this]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return this->resolveNeighbor(this->addNeighbor(in));
    });

    this->populateNeighborsCache(this->portDescriptor());
  };
  auto verify = [this]() {
    this->verifyNeighborCache(
        this->portDescriptor(), this->getProgrammedState());
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
class AgentNeighborOnMultiplePortsTest : public AgentHwTest {
  static auto constexpr isIntfNbrTable = EnableIntfNbrTableT::intfNbrTable;

 protected:
  void setCmdLineFlagOverrides() const override {
    FLAGS_intf_nbr_tables = isIntfNbrTable;
    AgentHwTest::setCmdLineFlagOverrides();
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    std::vector<ProductionFeature> features = {
        ProductionFeature::L3_FORWARDING};
    if (isIntfNbrTable) {
      features.push_back(ProductionFeature::INTERFACE_NEIGHBOR_TABLE);
    } else {
      features.push_back(ProductionFeature::VLAN);
    }
    return features;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(), ensemble.masterLogicalPortIds());
  }
  folly::IPAddressV6 neighborIP(PortID port) const {
    utility::EcmpSetupAnyNPorts6 ecmpHelper6(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    return ecmpHelper6.ip(PortDescriptor(port));
  }

  void oneNeighborPerPortSetup(const std::vector<PortID>& portIds) {
    auto cfg = initialConfig(*getAgentEnsemble());
    if (FLAGS_disable_loopback) {
      // Disable loopback on specific ports
      for (auto portId : portIds) {
        auto portCfg = utility::findCfgPortIf(cfg, portId);
        if (portCfg != cfg.ports()->end()) {
          portCfg->loopbackMode() = cfg::PortLoopbackMode::NONE;
        }
      }
      this->applyNewConfig(cfg);
    }

    // Create adjacencies on all test ports
    auto dstMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());
    for (int idx = 0; idx < portIds.size(); idx++) {
      this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
        utility::EcmpSetupAnyNPorts6 ecmpHelper6(
            in,
            getSw()->needL2EntryForNeighbor(),
            utility::MacAddressGenerator().get(dstMac.u64NBO() + idx + 1));
        return ecmpHelper6.resolveNextHops(in, {PortDescriptor(portIds[idx])});
      });
    }

    // Dump the local interface config
    XLOG(DBG0) << "Dumping port configurations:";
    for (int idx = 0; idx < portIds.size(); idx++) {
      auto mac =
          utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
      XLOG(DBG0) << "   Port " << portIds[idx]
                 << ", IPv6: " << cfg.interfaces()[idx].ipAddresses()[1]
                 << ", Intf MAC: " << mac;
    }
  }

  InterfaceID getInterfaceId(const PortID& portId) const {
    auto switchType =
        checkSameAndGetAsic(getAgentEnsemble()->getL3Asics())->getSwitchType();

    if (isIntfNbrTable || switchType == cfg::SwitchType::VOQ) {
      return InterfaceID(*getProgrammedState()
                              ->getPorts()
                              ->getNodeIf(portId)
                              ->getInterfaceIDs()
                              .begin());
    } else if (switchType == cfg::SwitchType::NPU) {
      return InterfaceID(
          static_cast<int>((*getProgrammedState()
                                 ->getPorts()
                                 ->getNodeIf(portId)
                                 ->getVlans()
                                 .begin())
                               .first));
    }

    XLOG(FATAL) << "Unexpected switch type " << static_cast<int>(switchType);
  }

  bool isProgrammedToCPU(PortID& portId, folly::IPAddress ip) {
    auto intfId = getInterfaceId(portId);
    auto neighborInfo = getNeighborInfo(intfId, ip, *getAgentEnsemble());
    return *neighborInfo.isProgrammedToCpu();
  }
};

TYPED_TEST_SUITE(AgentNeighborOnMultiplePortsTest, IntfNbrTableTypes);

TYPED_TEST(AgentNeighborOnMultiplePortsTest, ResolveOnTwoPorts) {
  auto setup = [&]() {
    this->oneNeighborPerPortSetup(
        {this->masterLogicalInterfacePortIds()[0],
         this->masterLogicalInterfacePortIds()[1]});
  };
  auto verify = [&]() {
    EXPECT_FALSE(this->isProgrammedToCPU(
        this->masterLogicalInterfacePortIds()[0],
        this->neighborIP(this->masterLogicalInterfacePortIds()[0])));
    EXPECT_FALSE(this->isProgrammedToCPU(
        this->masterLogicalInterfacePortIds()[1],
        this->neighborIP(this->masterLogicalInterfacePortIds()[1])));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

template <typename NeighborT>
class AgentNeighborMetadataTest : public AgentNeighborTest<NeighborT> {
 protected:
  using IPAddrT = typename NeighborT::IPAddrT;
  static auto constexpr programToTrunk = NeighborT::isTrunk;
  static auto constexpr isIntfNbrTable = NeighborT::isIntfNbrTable;
  using NTable = typename std::conditional_t<
      std::is_same<IPAddrT, folly::IPAddressV4>::value,
      ArpTable,
      NdpTable>;

 protected:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    std::vector<ProductionFeature> features = {
        ProductionFeature::L3_FORWARDING,
        ProductionFeature::CLASS_ID_FOR_NEIGHBOR};
    if (isIntfNbrTable) {
      features.push_back(ProductionFeature::INTERFACE_NEIGHBOR_TABLE);
    } else {
      features.push_back(ProductionFeature::VLAN);
    }
    if (programToTrunk) {
      features.push_back(ProductionFeature::LAG);
    }
    return features;
  }
};

TYPED_TEST_SUITE(AgentNeighborMetadataTest, NeighborTypes);

TYPED_TEST(
    AgentNeighborMetadataTest,
    ResolvePendingEntryThenChangeLookupClass) {
  auto setup = [this]() {
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto state = this->addNeighbor(in);
      return this->resolveNeighbor(state, this->kLookupClass);
    });

    this->verifyClassId(static_cast<int>(this->kLookupClass));
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return this->resolveNeighbor(in, this->kLookupClass2);
    });
  };
  auto verify = [this]() {
    EXPECT_FALSE(this->isProgrammedToCPU());
    this->verifyClassId(static_cast<int>(this->kLookupClass2));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
