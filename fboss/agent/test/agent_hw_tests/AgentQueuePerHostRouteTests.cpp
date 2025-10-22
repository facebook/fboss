/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

namespace {
const static int kVlanID = 2000;
const static int kInterfaceID = 2000;
} // namespace

namespace facebook::fboss {

template <typename AddrT>
class AgentQueuePerHostRouteTest : public AgentHwTest {
 public:
  using Type = AddrT;

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::oneL3IntfTwoPortConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds()[0],
        ensemble.masterLogicalPortIds()[1]);
    utility::addQueuePerHostQueueConfig(&cfg);
    utility::addQueuePerHostAcls(&cfg, ensemble.isSai());
    return cfg;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::QUEUE_PER_HOST};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // Enable neighbor cache so that class id is set
    FLAGS_disable_neighbor_updates = false;
  }

  bool isIntfNbrTable() const {
    return FLAGS_intf_nbr_tables;
  }

  RouterID kRouterID() const {
    return RouterID(0);
  }

  cfg::AclLookupClass kLookupClass() const {
    return cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
  }

  RoutePrefix<AddrT> kGetRoutePrefix() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return RoutePrefix<folly::IPAddressV4>{
          folly::IPAddressV4{"10.10.1.0"}, 24};
    } else {
      return RoutePrefix<folly::IPAddressV6>{
          folly::IPAddressV6{"2803:6080:d038:3063::"}, 64};
    }
  }

  void addRoutes(const std::vector<RoutePrefix<AddrT>>& routePrefixes) {
    auto kEcmpWidth = 1;
    utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor(), kRouterID());
    auto wrapper = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, kEcmpWidth, routePrefixes);
  }

  void resolveNeighborAndVerifyClassID(
      AddrT ipAddress,
      MacAddress macAddress,
      PortID port) {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      if (isIntfNbrTable()) {
        getSw()->getNeighborUpdater()->receivedArpMineForIntf(
            InterfaceID(kInterfaceID),
            ipAddress,
            macAddress,
            PortDescriptor(port),
            ArpOpCode::ARP_OP_REPLY);
      } else {
        getSw()->getNeighborUpdater()->receivedArpMine(
            VlanID(kVlanID),
            ipAddress,
            macAddress,
            PortDescriptor(port),
            ArpOpCode::ARP_OP_REPLY);
      }
      WITH_RETRIES({
        std::list<facebook::fboss::ArpEntryThrift> entries;

        if (isIntfNbrTable()) {
          entries =
              getSw()->getNeighborUpdater()->getArpCacheDataForIntf().get();
        } else {
          entries = getSw()->getNeighborUpdater()->getArpCacheData().get();
        }
        bool found = false;
        for (const auto& entry : entries) {
          if (entry.ip() == network::toBinaryAddress(ipAddress)) {
            found = true;
            EXPECT_EVENTUALLY_TRUE(*entry.classID());
          }
        }
        EXPECT_EVENTUALLY_TRUE(found);
      });
    } else {
      if (isIntfNbrTable()) {
        getSw()->getNeighborUpdater()->receivedNdpMineForIntf(
            InterfaceID(kInterfaceID),
            ipAddress,
            macAddress,
            PortDescriptor(port),
            ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
            0);
      } else {
        getSw()->getNeighborUpdater()->receivedNdpMine(
            VlanID(kVlanID),
            ipAddress,
            macAddress,
            PortDescriptor(port),
            ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
            0);
      }
      WITH_RETRIES({
        std::list<facebook::fboss::NdpEntryThrift> entries;
        if (isIntfNbrTable()) {
          entries =
              getSw()->getNeighborUpdater()->getNdpCacheDataForIntf().get();
        } else {
          entries = getSw()->getNeighborUpdater()->getNdpCacheData().get();
        }
        bool found = false;
        for (const auto& entry : entries) {
          if (entry.ip() == network::toBinaryAddress(ipAddress)) {
            found = true;
            EXPECT_EVENTUALLY_TRUE(*entry.classID());
          }
        }
        EXPECT_EVENTUALLY_TRUE(found);
      });
    }
  }

  AddrT kSrcIP() {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return folly::IPAddressV4("1.0.0.1");
    } else {
      return folly::IPAddressV6("1::1");
    }
  }

  AddrT kDstIP() {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return folly::IPAddressV4("10.10.1.2");
    } else {
      return folly::IPAddressV6("2803:6080:d038:3063::2");
    }
  }

  std::vector<std::pair<AddrT, MacAddress>> kNeighborIPs() {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return {
          {folly::IPAddressV4("1.1.1.4"), MacAddress("06:00:00:00:00:03")},
          {folly::IPAddressV4("1.1.1.3"), MacAddress("06:00:00:00:00:02")},
          {folly::IPAddressV4("1.1.1.2"), MacAddress("06:00:00:00:00:01")}};
    } else {
      return {
          {folly::IPAddressV6("1::3"), MacAddress("06:00:00:00:00:03")},
          {folly::IPAddressV6("1::2"), MacAddress("06:00:00:00:00:02")},
          {folly::IPAddressV6("1::1"), MacAddress("06:00:00:00:00:01")}};
    }
  }

  void setMacAddrsToBlock() {
    auto cfgMacAddrsToBlock = std::make_unique<std::vector<cfg::MacAndVlan>>();
    for (const auto& ipAndMac : kNeighborIPs()) {
      cfg::MacAndVlan macAndVlan;
      macAndVlan.vlanID() = kVlanID;
      macAndVlan.macAddress() = ipAndMac.second.toString();
      cfgMacAddrsToBlock->emplace_back(macAndVlan);
    }
    ThriftHandler handler(getSw());
    handler.setMacAddrsToBlock(std::move(cfgMacAddrsToBlock));
  }

  void setupHelper(bool blockNeighbor) {
    // Disable neigbor updates to prevent stats increment
    FLAGS_disable_neighbor_updates = true;
    this->getAgentEnsemble()->bringDownPort(this->masterLogicalPortIds()[1]);
    if (blockNeighbor) {
      this->setMacAddrsToBlock();
    }
    for (const auto& neighborIpAndMac : kNeighborIPs()) {
      this->resolveNeighborAndVerifyClassID(
          neighborIpAndMac.first,
          neighborIpAndMac.second,
          this->masterLogicalPortIds()[0]);
    }
    this->addRoutes({this->kGetRoutePrefix()});
  }

  void verifyHelper(bool useFrontPanel, bool blockNeighbor) {
    // Disable neigbor updates to prevent stats increment
    FLAGS_disable_neighbor_updates = true;
    this->getNextUpdatedPortStats(this->masterLogicalPortIds()[0]);
    XLOG(DBG2) << "verify send packets "
               << (useFrontPanel ? "out of port" : "switched");
    auto vlanId = VlanID(*this->initialConfig(*this->getAgentEnsemble())
                              .vlanPorts()[0]
                              .vlanID());
    auto intfMac = utility::getInterfaceMac(this->getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);

    utility::verifyQueuePerHostMapping(
        getAgentEnsemble(),
        vlanId,
        srcMac,
        intfMac,
        this->kSrcIP(),
        this->kDstIP(),
        useFrontPanel,
        blockNeighbor,
        std::nullopt, /* l4SrcPort */
        std::nullopt, /* l4DstPort */
        std::nullopt); /* dscp */
  }
};

using IpTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_SUITE(AgentQueuePerHostRouteTest, IpTypes);

TYPED_TEST(AgentQueuePerHostRouteTest, VerifyHostToQueueMappingClassID) {
  auto setup = [=, this]() { this->setupHelper(false /* blockNeighbor */); };
  auto verify = [=, this]() {
    this->getAgentEnsemble()->bringUpPort(this->masterLogicalPortIds()[1]);
    this->verifyHelper(true /* front panel port */, false /* block neighbor */);
    this->getAgentEnsemble()->bringDownPort(this->masterLogicalPortIds()[1]);
    this->verifyHelper(false /* cpu port */, false /* block neighbor */);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentQueuePerHostRouteTest, VerifyHostToQueueMappingClassIDBlock) {
  auto setup = [=, this]() { this->setupHelper(true /* blockNeighbor */); };
  auto verify = [=, this]() {
    this->getAgentEnsemble()->bringUpPort(this->masterLogicalPortIds()[1]);
    this->verifyHelper(true /* front panel port */, true /* block neighbor */);
    this->getAgentEnsemble()->bringDownPort(this->masterLogicalPortIds()[1]);
    this->verifyHelper(false /* cpu port */, true /* block neighbor */);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
