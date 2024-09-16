/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "common/network/if/gen-cpp2/Address_types.h"
#include "common/stats/MonotonicCounter.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Transceiver.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

using namespace facebook::fboss;
using namespace facebook::stats;
using apache::thrift::TEnumTraits;
using cfg::PortSpeed;
using facebook::network::toBinaryAddress;
using facebook::network::thrift::BinaryAddress;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::StringPiece;
using std::shared_ptr;
using std::unique_ptr;
using std::chrono::duration_cast;
using ::testing::_;
using ::testing::Return;
using testing::UnorderedElementsAreArray;

namespace {
static TeCounterID kCounterID("counter0");
static std::string kNhopAddrA("2401:db00:2110:3001::0002");
static std::string kNhopAddrB("2401:db00:2110:3055::0002");
static folly::MacAddress kMacAddress("01:02:03:04:05:06");
static VlanID kVlanA(1);
static VlanID kVlanB(55);
static InterfaceID kInterfaceA(1);
static InterfaceID kInterfaceB(55);
static PortID kPortIDA(1);
static PortID kPortIDB(11);

IpPrefix ipPrefix(StringPiece ip, int length) {
  IpPrefix result;
  result.ip() = toBinaryAddress(IPAddress(ip));
  result.prefixLength() = length;
  return result;
}

IpPrefix ipPrefix(const folly::CIDRNetwork& nw) {
  IpPrefix result;
  result.ip() = toBinaryAddress(nw.first);
  result.prefixLength() = nw.second;
  return result;
}

FlowEntry makeFlow(
    std::string dstIp,
    std::string nhip = kNhopAddrA,
    std::string counter = "counter0",
    std::string nhopAddr = "fboss1") {
  FlowEntry flowEntry;
  flowEntry.flow()->srcPort() = 100;
  flowEntry.flow()->dstPrefix() = ipPrefix(dstIp, 64);
  flowEntry.counterID() = counter;
  std::vector<NextHopThrift> nexthops;
  NextHopThrift nexthop;
  nexthop.address() = toBinaryAddress(IPAddress(nhip));
  nexthop.address()->ifName() = nhopAddr;
  nexthops.push_back(nexthop);
  flowEntry.nexthops() = nexthops;
  return flowEntry;
}
} // unnamed namespace

class ThriftTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
  }
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TEST_F(ThriftTest, getInterfaceDetail) {
  ThriftHandler handler(this->sw_);

  // Query the two interfaces configured by testStateA()
  InterfaceDetail info;
  handler.getInterfaceDetail(info, 1);
  EXPECT_EQ("eth1/5/1", *info.interfaceName());
  EXPECT_EQ(1, *info.interfaceId());
  EXPECT_EQ(1, *info.vlanId());
  EXPECT_EQ(0, *info.routerId());
  EXPECT_EQ("00:02:00:00:00:01", *info.mac());
  std::vector<IpPrefix> expectedAddrs = {
      ipPrefix("10.0.0.1", 24),
      ipPrefix("192.168.0.1", 24),
      ipPrefix("2401:db00:2110:3001::0001", 64),
      ipPrefix("fe80::202:ff:fe00:1", 64),
      ipPrefix("fe80::", 64),
  };
  EXPECT_THAT(*info.address(), UnorderedElementsAreArray(expectedAddrs));

  handler.getInterfaceDetail(info, 55);
  EXPECT_EQ("eth1/6/1", *info.interfaceName());
  EXPECT_EQ(55, *info.interfaceId());
  EXPECT_EQ(55, *info.vlanId());
  EXPECT_EQ(0, *info.routerId());
  EXPECT_EQ("00:02:00:00:00:55", *info.mac());
  expectedAddrs = {
      ipPrefix("10.0.55.1", 24),
      ipPrefix("192.168.55.1", 24),
      ipPrefix("2401:db00:2110:3055::0001", 64),
      ipPrefix("fe80::202:ff:fe00:55", 64),
      ipPrefix("169.254.0.0", 16),
  };
  EXPECT_THAT(*info.address(), UnorderedElementsAreArray(expectedAddrs));

  // Calling getInterfaceDetail() on an unknown
  // interface should throw an FbossError.
  EXPECT_THROW(handler.getInterfaceDetail(info, 123), FbossError);
}

template <typename SwitchTypeAndEnableIntfNbrTableT>
class ThriftTestAllSwitchTypes : public ::testing::Test {
 public:
  static auto constexpr switchType =
      SwitchTypeAndEnableIntfNbrTableT::switchType;
  static auto constexpr intfNbrTable =
      SwitchTypeAndEnableIntfNbrTableT::intfNbrTable;
  ;
  void SetUp() override {
    FLAGS_intf_nbr_tables = intfNbrTable;
    FLAGS_dsf_num_parallel_sessions_per_remote_interface_node =
        std::numeric_limits<uint32_t>::max();
    auto config = testConfigA(switchType);
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());
  }
  bool isVoq() const {
    return switchType == cfg::SwitchType::VOQ;
  }
  bool isFabric() const {
    return switchType == cfg::SwitchType::FABRIC;
  }
  bool isNpu() const {
    return switchType == cfg::SwitchType::NPU;
  }
  int interfaceIdBegin() const {
    auto switchId = getSwitchIdAndType().first;
    return isVoq() ? *sw_->getState()
                          ->getDsfNodes()
                          ->getNodeIf(switchId)
                          ->getSystemPortRange()
                          ->minimum() +
            5
                   : 1;
  }

  std::pair<SwitchID, cfg::SwitchType> getSwitchIdAndType() const {
    if (isNpu()) {
      return {SwitchID(kNpuSwitchIdBegin), cfg::SwitchType::NPU};
    } else if (isFabric()) {
      return {SwitchID(kFabricSwitchIdBegin), cfg::SwitchType::FABRIC};
    } else if (isVoq()) {
      return {SwitchID(kVoqSwitchIdBegin), cfg::SwitchType::VOQ};
    }
    throw FbossError("Invalid switch type");
  }
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TYPED_TEST_SUITE(ThriftTestAllSwitchTypes, SwitchTypeAndEnableIntfNbrTable);

TYPED_TEST(ThriftTestAllSwitchTypes, checkSwitchId) {
  auto switchInfoTable = this->sw_->getSwitchInfoTable();
  auto switchIdAndType = this->getSwitchIdAndType();
  for (const auto& type :
       {cfg::SwitchType::NPU, cfg::SwitchType::VOQ, cfg::SwitchType::FABRIC}) {
    if (type == switchIdAndType.second) {
      EXPECT_EQ(
          *switchInfoTable.getSwitchIdsOfType(switchIdAndType.second).begin(),
          switchIdAndType.first);
    } else {
      EXPECT_EQ(switchInfoTable.getSwitchIdsOfType(type).size(), 0);
    }
  }
  auto hwAsicTable = this->sw_->getHwAsicTable();
  EXPECT_NE(hwAsicTable, nullptr);
  auto hwAsic = hwAsicTable->getHwAsicIf(switchIdAndType.first);
  EXPECT_NE(hwAsic, nullptr);
  if (this->isVoq()) {
    EXPECT_EQ(hwAsic->getAsicMac(), folly::MacAddress("02:00:00:00:0F:0B"));
  }
  if (this->isFabric() || this->isVoq()) {
    EXPECT_EQ(
        *(switchInfoTable.getSwitchIdToSwitchInfo()
              .at(switchIdAndType.first)

              .connectionHandle()),
        "68:00");
  }
  EXPECT_EQ(SwitchID(*hwAsic->getSwitchId()), switchIdAndType.first);
  EXPECT_EQ(hwAsic->getSwitchType(), switchIdAndType.second);
  auto config = testConfigA(switchIdAndType.second);
  EXPECT_EQ(
      *(this->sw_->getScopeResolver()
            ->scope(PortID(*(config.ports()[0].logicalID())))
            .switchIds()
            .begin()),
      switchIdAndType.first);
}

TYPED_TEST(ThriftTestAllSwitchTypes, listHwObjects) {
  ThriftHandler handler(this->sw_);
  std::string out;
  std::vector<HwObjectType> in{HwObjectType::PORT};
  EXPECT_HW_CALL(this->sw_, listObjects(in, testing::_)).Times(1);
  handler.listHwObjects(
      out, std::make_unique<std::vector<HwObjectType>>(in), false);
}

TYPED_TEST(ThriftTestAllSwitchTypes, getHwDebugDump) {
  ThriftHandler handler(this->sw_);
  std::string out;
  EXPECT_HW_CALL(this->sw_, dumpDebugState(testing::_)).Times(1);
  // Mock getHwDebugDump doesn't write any thing so expect FbossError
  EXPECT_THROW(handler.getHwDebugDump(out), FbossError);
}

TYPED_TEST(ThriftTestAllSwitchTypes, getL2Table) {
  ThriftHandler handler(this->sw_);
  std::string out;
  EXPECT_HW_CALL(this->sw_, fetchL2Table(testing::_))
      .Times(this->isNpu() ? 1 : 0);

  std::vector<L2EntryThrift> l2Entries;
  if (this->isNpu()) {
    handler.getL2Table(l2Entries);
  } else {
    EXPECT_THROW(handler.getL2Table(l2Entries), FbossError);
  }
}

TEST(ThriftEnum, assertPortSpeeds) {
  // We rely on the exact value of the port speeds for some
  // logic, so we want to ensure that these values don't change.
  for (const auto key : TEnumTraits<cfg::PortSpeed>::values) {
    switch (key) {
      case PortSpeed::DEFAULT:
        continue;
      case PortSpeed::GIGE:
        EXPECT_EQ(static_cast<int>(key), 1000);
        break;
      case PortSpeed::XG:
        EXPECT_EQ(static_cast<int>(key), 10000);
        break;
      case PortSpeed::TWENTYG:
        EXPECT_EQ(static_cast<int>(key), 20000);
        break;
      case PortSpeed::TWENTYFIVEG:
        EXPECT_EQ(static_cast<int>(key), 25000);
        break;
      case PortSpeed::FORTYG:
        EXPECT_EQ(static_cast<int>(key), 40000);
        break;
      case PortSpeed::FIFTYG:
        EXPECT_EQ(static_cast<int>(key), 50000);
        break;
      case PortSpeed::FIFTYTHREEPOINTONETWOFIVEG:
        EXPECT_EQ(static_cast<int>(key), 53125);
        break;
      case PortSpeed::HUNDREDG:
        EXPECT_EQ(static_cast<int>(key), 100000);
        break;
      case PortSpeed::HUNDREDANDSIXPOINTTWOFIVEG:
        EXPECT_EQ(static_cast<int>(key), 106250);
        break;
      case PortSpeed::TWOHUNDREDG:
        EXPECT_EQ(static_cast<int>(key), 200000);
        break;
      case PortSpeed::FOURHUNDREDG:
        EXPECT_EQ(static_cast<int>(key), 400000);
        break;
      case PortSpeed::EIGHTHUNDREDG:
        EXPECT_EQ(static_cast<int>(key), 800000);
        break;
    }
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, LinkLocalRoutes) {
  // Link local addr.
  auto ip = IPAddressV6("fe80::");
  // Find longest match to link local addr.
  auto longestMatchRoute = findLongestMatchRoute(
      this->sw_->getRib(), RouterID(0), ip, this->sw_->getState());
  if (this->isFabric()) {
    ASSERT_EQ(nullptr, longestMatchRoute);
  } else {
    // Verify that a route is found. Link local route should always
    // be present
    ASSERT_NE(nullptr, longestMatchRoute);
    // Verify that the route is to link local addr.
    ASSERT_EQ(longestMatchRoute->prefix().network(), ip);
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, flushNonExistentNeighbor) {
  ThriftHandler handler(this->sw_);
  auto v4Addr = std::make_unique<BinaryAddress>(
      toBinaryAddress(IPAddress("100.100.100.1")));
  auto v6Addr =
      std::make_unique<BinaryAddress>(toBinaryAddress(IPAddress("100::100")));
  if (this->isNpu()) {
    EXPECT_EQ(handler.flushNeighborEntry(std::move(v4Addr), 1), 0);
    EXPECT_EQ(handler.flushNeighborEntry(std::move(v6Addr), 1), 0);
  } else {
    EXPECT_THROW(
        handler.flushNeighborEntry(std::move(v4Addr), 1001), FbossError);
    EXPECT_THROW(
        handler.flushNeighborEntry(std::move(v6Addr), 1001), FbossError);
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, setPortState) {
  const PortID port5{5};
  ThriftHandler handler(this->sw_);
  handler.setPortState(port5, true);
  this->sw_->linkStateChanged(port5, true);
  waitForStateUpdates(this->sw_);

  auto port = this->sw_->getState()->getPorts()->getNodeIf(port5);
  EXPECT_TRUE(port->isUp());
  EXPECT_TRUE(port->isEnabled());

  this->sw_->linkStateChanged(port5, false);
  handler.setPortState(port5, false);
  waitForStateUpdates(this->sw_);

  port = this->sw_->getState()->getPorts()->getNodeIf(port5);
  EXPECT_FALSE(port->isUp());
  EXPECT_FALSE(port->isEnabled());
}

TYPED_TEST(ThriftTestAllSwitchTypes, setPortDrainState) {
  const PortID port5{5};
  auto port = this->sw_->getState()->getPorts()->getNodeIf(port5);
  EXPECT_FALSE(port->isDrained());

  ThriftHandler handler(this->sw_);
  if (this->isFabric()) {
    handler.setPortDrainState(port5, true);
    waitForStateUpdates(this->sw_);
    port = this->sw_->getState()->getPorts()->getNodeIf(port5);
    EXPECT_TRUE(port->isDrained());

    handler.setPortDrainState(port5, false);
    waitForStateUpdates(this->sw_);
    port = this->sw_->getState()->getPorts()->getNodeIf(port5);
    EXPECT_FALSE(port->isDrained());
  } else {
    EXPECT_THROW(handler.setPortDrainState(port5, true), FbossError);
    EXPECT_THROW(handler.setPortDrainState(port5, false), FbossError);
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getPortStatus) {
  const PortID port5{5};
  auto port = this->sw_->getState()->getPorts()->getNodeIf(port5);
  ThriftHandler handler(this->sw_);

  std::map<int32_t, PortStatus> statusMap;
  std::vector<int> ports{5};
  handler.getPortStatus(statusMap, std::make_unique<std::vector<int>>(ports));
  auto portStatus = statusMap.find(5);
  EXPECT_FALSE(*portStatus->second.drained());

  if (this->isFabric()) {
    handler.setPortDrainState(port5, true);
    waitForStateUpdates(this->sw_);
    handler.getPortStatus(statusMap, std::make_unique<std::vector<int>>(ports));
    portStatus = statusMap.find(5);
    EXPECT_TRUE(*portStatus->second.drained());
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getSetSwitchDrainState) {
  ThriftHandler handler(this->sw_);
  auto switchDrainFn =
      [this](const shared_ptr<SwitchState>& state) -> shared_ptr<SwitchState> {
    shared_ptr<SwitchState> newState{state};
    auto oldSwitchSettings =
        utility::getFirstNodeIf(state->getSwitchSettings());
    auto newSwitchSettings = oldSwitchSettings->modify(&newState);
    newSwitchSettings->setSwitchDrainState(cfg::SwitchDrainState::DRAINED);
    return newState;
  };

  if (this->isFabric()) {
    EXPECT_FALSE(handler.isSwitchDrained());
    this->sw_->updateStateBlocking("Switch drain", switchDrainFn);
    EXPECT_TRUE(handler.isSwitchDrained());
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getAndSetNeighborsToBlock) {
  ThriftHandler handler(this->sw_);

  auto blockListVerify =
      [this, &handler](
          std::vector<std::pair<VlanID, folly::IPAddress>> neighborsToBlock) {
        auto cfgNeighborsToBlock =
            std::make_unique<std::vector<cfg::Neighbor>>();

        for (const auto& [vlanID, ipAddress] : neighborsToBlock) {
          cfg::Neighbor neighbor;
          neighbor.vlanID() = vlanID;
          neighbor.ipAddress() = ipAddress.str();
          cfgNeighborsToBlock->emplace_back(neighbor);
        }
        auto expectedCfgNeighborsToBlock = *cfgNeighborsToBlock;
        if (this->isNpu()) {
          handler.setNeighborsToBlock(std::move(cfgNeighborsToBlock));
        } else {
          EXPECT_THROW(
              handler.setNeighborsToBlock(std::move(cfgNeighborsToBlock)),
              FbossError);
        }
        waitForStateUpdates(handler.getSw());

        auto gotBlockedNeighbors =
            utility::getFirstNodeIf(
                handler.getSw()->getState()->getSwitchSettings())
                ->getBlockNeighbors_DEPRECATED();

        std::vector<cfg::Neighbor> gotBlockedNeighborsViaThrift;
        handler.getBlockedNeighbors(gotBlockedNeighborsViaThrift);
        if (this->isNpu()) {
          EXPECT_EQ(neighborsToBlock, gotBlockedNeighbors);
          EXPECT_EQ(gotBlockedNeighborsViaThrift, expectedCfgNeighborsToBlock);
        } else {
          std::vector<std::pair<VlanID, folly::IPAddress>> expectedBlockedNbrs;
          EXPECT_EQ(expectedBlockedNbrs, gotBlockedNeighbors);
          EXPECT_EQ(std::vector<cfg::Neighbor>(), gotBlockedNeighborsViaThrift);
        }
      };

  // set blockneighbor1
  blockListVerify(
      {{VlanID(2000), folly::IPAddress("2401:db00:2110:3001::0003")}});

  // set blockneighbor1, blockNeighbor2
  blockListVerify(
      {{VlanID(2000), folly::IPAddress("2401:db00:2110:3001::0003")},
       {VlanID(2000), folly::IPAddress("2401:db00:2110:3001::0004")}});

  // set blockNeighbor2
  blockListVerify(
      {{VlanID(2000), folly::IPAddress("2401:db00:2110:3001::0004")}});
  auto setNeighborsToBlock =
      [this, &handler](std::unique_ptr<std::vector<cfg::Neighbor>> toBlock) {
        if (this->isNpu()) {
          handler.setNeighborsToBlock(std::move(toBlock));
          waitForStateUpdates(this->sw_);
        } else {
          EXPECT_THROW(
              handler.setNeighborsToBlock(std::move(toBlock)), FbossError);
        }
      };
  // set null list (clears block list)
  std::vector<cfg::Neighbor> blockedNeighbors;
  setNeighborsToBlock({});
  EXPECT_EQ(
      0,
      utility::getFirstNodeIf(this->sw_->getState()->getSwitchSettings())
          ->getBlockNeighbors_DEPRECATED()
          .size());
  handler.getBlockedNeighbors(blockedNeighbors);
  EXPECT_TRUE(blockedNeighbors.empty());
  // set empty list (clears block list)
  auto neighborsToBlock = std::make_unique<std::vector<cfg::Neighbor>>();
  setNeighborsToBlock(std::move(neighborsToBlock));
  EXPECT_EQ(
      0,
      utility::getFirstNodeIf(this->sw_->getState()->getSwitchSettings())
          ->getBlockNeighbors_DEPRECATED()
          .size());
  handler.getBlockedNeighbors(blockedNeighbors);
  EXPECT_TRUE(blockedNeighbors.empty());

  auto invalidNeighborToBlock =
      "twshared12345.06.abc7"; // only IPs are supported
  auto invalidNeighborsToBlock = std::make_unique<std::vector<cfg::Neighbor>>();
  cfg::Neighbor neighbor;
  neighbor.vlanID() = 2000;
  neighbor.ipAddress() = invalidNeighborToBlock;
  invalidNeighborsToBlock->emplace_back(neighbor);
  EXPECT_THROW(
      handler.setNeighborsToBlock(std::move(invalidNeighborsToBlock)),
      FbossError);
  handler.getBlockedNeighbors(blockedNeighbors);
  EXPECT_TRUE(blockedNeighbors.empty());
}

TYPED_TEST(ThriftTestAllSwitchTypes, getDsfNodes) {
  ThriftHandler handler(this->sw_);
  std::map<int64_t, cfg::DsfNode> dsfNodes;
  if (this->isNpu()) {
    EXPECT_THROW(handler.getDsfNodes(dsfNodes), FbossError);
  } else {
    handler.getDsfNodes(dsfNodes);
    EXPECT_EQ(
        dsfNodes.size(), this->sw_->getState()->getDsfNodes()->numNodes());
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getSysPorts) {
  ThriftHandler handler(this->sw_);
  std::map<int64_t, SystemPortThrift> sysPorts;
  handler.getSystemPorts(sysPorts);
  if (this->isVoq()) {
    EXPECT_GT(sysPorts.size(), 1);
    EXPECT_EQ(
        sysPorts.size(),
        this->sw_->getState()->getSystemPorts()->numNodes() +
            this->sw_->getState()->getRemoteSystemPorts()->numNodes());
  } else {
    EXPECT_EQ(sysPorts.size(), 0);
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getSysPortStats) {
  ThriftHandler handler(this->sw_);
  std::map<std::string, HwSysPortStats> sysPortStats;
  EXPECT_HW_CALL(this->sw_, getSysPortStats()).Times(1);
  this->sw_->updateStats();
  handler.getSysPortStats(sysPortStats);
}

TYPED_TEST(ThriftTestAllSwitchTypes, getHwPortStats) {
  ThriftHandler handler(this->sw_);
  std::map<std::string, HwPortStats> hwPortStats;
  EXPECT_HW_CALL(this->sw_, getPortStats()).Times(1);
  this->sw_->updateStats();
  handler.getHwPortStats(hwPortStats);
}

TYPED_TEST(ThriftTestAllSwitchTypes, getFabricReachabilityStats) {
  ThriftHandler handler(this->sw_);
  FabricReachabilityStats stats;
  handler.getFabricReachabilityStats(stats);
}

TYPED_TEST(ThriftTestAllSwitchTypes, getCpuPortStats) {
  ThriftHandler handler(this->sw_);
  CpuPortStats cpuPortStats;
  EXPECT_HW_CALL(this->sw_, getCpuPortStats()).Times(1);
  this->sw_->updateStats();
  handler.getCpuPortStats(cpuPortStats);
}

TYPED_TEST(ThriftTestAllSwitchTypes, getAllEcmpDetails) {
  ThriftHandler handler(this->sw_);
  std::vector<EcmpDetails> ecmpDetails;
  EXPECT_HW_CALL(this->sw_, getAllEcmpDetails()).Times(1);
  handler.getAllEcmpDetails(ecmpDetails);
}

TYPED_TEST(ThriftTestAllSwitchTypes, getAclTableGroup) {
  SCOPE_EXIT {
    FLAGS_enable_acl_table_group = false;
  };
  FLAGS_enable_acl_table_group = true;
  ThriftHandler handler(this->sw_);

  auto switchIdAndType = this->getSwitchIdAndType();

  AclTableThrift aclTables;
  handler.getAclTableGroup(aclTables);
  EXPECT_EQ(aclTables.aclTableEntries()->size(), 0);
  // No ACLs on fabric switches
  if (!this->isFabric()) {
    cfg::SwitchConfig config = testConfigA(switchIdAndType.second);
    cfg::AclTableGroup tableGroup;
    tableGroup.name() = "test-table-group";
    tableGroup.stage() = cfg::AclStage::INGRESS;

    auto createAclTable = [](auto tableNum) {
      cfg::AclTable cfgTable;

      cfgTable.name() = folly::to<std::string>("test-table-", tableNum);
      cfgTable.priority() = tableNum;
      cfgTable.aclEntries()->resize(2);
      cfgTable.aclEntries()[0].name() =
          folly::to<std::string>("table", tableNum, "_acl1");
      cfgTable.aclEntries()[0].actionType() = cfg::AclActionType::DENY;
      cfgTable.aclEntries()[0].srcIp() = "192.168.0.1";
      cfgTable.aclEntries()[0].dstIp() = "192.168.0.0/24";
      cfgTable.aclEntries()[0].srcPort() = 5;
      cfgTable.aclEntries()[0].dstPort() = 8;
      cfgTable.aclEntries()[1].name() =
          folly::to<std::string>("table", tableNum, "_acl2");
      cfgTable.aclEntries()[1].actionType() = cfg::AclActionType::DENY;
      cfgTable.aclEntries()[1].srcIp() = "192.168.1.1";
      cfgTable.aclEntries()[1].dstIp() = "192.168.1.0/24";
      cfgTable.aclEntries()[1].srcPort() = 5;
      cfgTable.aclEntries()[1].dstPort() = 8;

      return cfgTable;
    };

    tableGroup.aclTables()->push_back(createAclTable(1));
    tableGroup.aclTables()->push_back(createAclTable(2));

    config.aclTableGroup() = tableGroup;
    this->sw_->applyConfig("New config with acl table group", config);
    auto state = this->sw_->getState();
    handler.getAclTableGroup(aclTables);
    EXPECT_EQ(aclTables.aclTableEntries()->size(), 2);
    int tableNum = 1;
    for (auto& [aclTableName, aclEntries] : *aclTables.aclTableEntries()) {
      EXPECT_EQ(aclTableName, folly::to<std::string>("test-table-", tableNum));
      EXPECT_EQ(aclEntries.size(), 2);
      for (int i = 0; i < 2; i++) {
        EXPECT_EQ(
            *aclEntries[i].name(),
            folly::to<std::string>("table", tableNum, "_acl", i + 1));
      }
      tableNum++;
    }
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getAclTable) {
  ThriftHandler handler(this->sw_);

  auto switchIdAndType = this->getSwitchIdAndType();

  std::vector<AclEntryThrift> aclTable;
  handler.getAclTable(aclTable);
  EXPECT_EQ(aclTable.size(), 0);
  // No ACLs on fabric switches
  if (!this->isFabric()) {
    cfg::SwitchConfig config = testConfigA(switchIdAndType.second);
    config.acls()->resize(2);
    config.acls()[0].name() = "acl1";
    config.acls()[0].actionType() = cfg::AclActionType::DENY;
    config.acls()[0].srcIp() = "192.168.0.1";
    config.acls()[0].dstIp() = "192.168.0.0/24";
    config.acls()[0].srcPort() = 5;
    config.acls()[0].dstPort() = 8;
    config.acls()[1].name() = "acl2";
    config.acls()[1].actionType() = cfg::AclActionType::DENY;
    config.acls()[1].srcIp() = "192.168.1.1";
    config.acls()[1].dstIp() = "192.168.1.0/24";
    config.acls()[1].srcPort() = 5;
    config.acls()[1].dstPort() = 8;
    this->sw_->applyConfig("New config with acls", config);
    auto state = this->sw_->getState();
    handler.getAclTable(aclTable);
    EXPECT_EQ(aclTable.size(), 2);
    EXPECT_EQ(*aclTable[0].name(), "acl1");
    EXPECT_EQ(*aclTable[0].srcPort(), 5);
    EXPECT_EQ(*aclTable[0].dstPort(), 8);
    EXPECT_EQ(*aclTable[0].actionType(), "deny");
    EXPECT_EQ(*aclTable[1].name(), "acl2");
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getSwitchReachability) {
  ThriftHandler handler(this->sw_);
  std::unique_ptr<std::vector<std::string>> switchNames =
      std::make_unique<std::vector<std::string>>();
  switchNames->push_back("dsfNodeCfg0");
  std::map<std::string, std::vector<std::string>> reachabilityMatrix;
  if (this->isNpu()) {
    EXPECT_HW_CALL(this->sw_, getSwitchReachability(testing::_)).Times(0);
    EXPECT_THROW(
        handler.getSwitchReachability(
            reachabilityMatrix, std::move(switchNames)),
        FbossError);
  } else {
    EXPECT_HW_CALL(this->sw_, getSwitchReachability(testing::_)).Times(1);
    handler.getSwitchReachability(reachabilityMatrix, std::move(switchNames));
    EXPECT_EQ(reachabilityMatrix.size(), 1);
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getDsfSubscriptions) {
  ThriftHandler handler(this->sw_);
  std::vector<FsdbSubscriptionThrift> subscriptions;
  if (this->isNpu()) {
    EXPECT_THROW(handler.getDsfSubscriptions(subscriptions), FbossError);
  } else if (this->isFabric()) {
    handler.getDsfSubscriptions(subscriptions);
    EXPECT_EQ(subscriptions.size(), 0);
  } else {
    // VOQ
    handler.getDsfSubscriptions(subscriptions);
    EXPECT_EQ(subscriptions.size(), 0);

    // Add 1 IN node to DSF config
    cfg::SwitchConfig config = testConfigA(cfg::SwitchType::VOQ);
    auto dsfNodeCfg = makeDsfNodeCfg(5);
    config.dsfNodes()->insert({5, dsfNodeCfg});
    this->sw_->applyConfig("Config with 1 more IN node", config);

    handler.getDsfSubscriptions(subscriptions);
    EXPECT_EQ(subscriptions.size(), 2);
    for (const auto& subscription : subscriptions) {
      EXPECT_EQ(*subscription.name(), *dsfNodeCfg.name());
      EXPECT_EQ((*subscription.paths()).size(), 3);
      EXPECT_EQ(*subscription.state(), "DISCONNECTED");
    }
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getDsfSubscriptionClientId) {
  ThriftHandler handler(this->sw_);
  std::string ret;
  if (this->isNpu()) {
    EXPECT_THROW(handler.getDsfSubscriptionClientId(ret), FbossError);
  } else {
    handler.getDsfSubscriptionClientId(ret);
    EXPECT_TRUE(ret.find(":agent") != std::string::npos);
  }
}

std::unique_ptr<UnicastRoute> makeUnicastRoute(
    std::string prefixStr,
    std::string nxtHop,
    AdminDistance distance = AdminDistance::MAX_ADMIN_DISTANCE,
    std::optional<RouteCounterID> counterID = std::nullopt,
    std::optional<cfg::AclLookupClass> classID = std::nullopt) {
  std::vector<std::string> vec;
  folly::split('/', prefixStr, vec);
  EXPECT_EQ(2, vec.size());
  auto nr = std::make_unique<UnicastRoute>();
  *nr->dest()->ip() = toBinaryAddress(IPAddress(vec.at(0)));
  *nr->dest()->prefixLength() = folly::to<uint8_t>(vec.at(1));
  nr->nextHopAddrs()->push_back(toBinaryAddress(IPAddress(nxtHop)));
  nr->adminDistance() = distance;
  if (counterID.has_value()) {
    nr->counterID() = *counterID;
  }
  if (classID.has_value()) {
    nr->classID() = *classID;
  }
  return nr;
}

// Test for the ThriftHandler::syncFib method
TYPED_TEST(ThriftTestAllSwitchTypes, multipleClientSyncFib) {
  if (this->isFabric()) {
    // no FIB on fabric or phy
    GTEST_SKIP();
  }
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(this->sw_);

  auto kIntf1 = InterfaceID(this->interfaceIdBegin());

  // Two clients - BGP and OPENR
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto openrClient = static_cast<int16_t>(ClientID::OPENR);
  auto bgpClientAdmin = this->sw_->clientIdToAdminDistance(bgpClient);
  auto openrClientAdmin = this->sw_->clientIdToAdminDistance(openrClient);

  // nhops to use
  std::string nhop4, nhop6;
  if (this->isVoq()) {
    nhop4 = "10.0.5.2";
    nhop6 = "2401:db00:2110:3005::0002";
  } else {
    nhop4 = "10.0.0.2";
    nhop6 = "2401:db00:2110:3001::0002";
  }

  // resolve the nexthops
  auto nh1 = makeResolvedNextHops({{kIntf1, nhop4}});
  auto nh2 = makeResolvedNextHops({{kIntf1, nhop6}});

  // prefixes to add
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  auto prefixB4 = "7.2.0.0/16";
  auto prefixB6 = "aaaa:2::0/64";
  auto prefixC4 = "7.3.0.0/16";
  auto prefixC6 = "aaaa:3::0/64";
  auto prefixD4 = "7.4.0.0/16";
  auto prefixD6 = "aaaa:4::0/64";

  auto addRoutesForClient = [&](const auto& prefix4,
                                const auto& prefix6,
                                const auto& client,
                                const auto& clientAdmin) {
    if (this->isFabric()) {
      EXPECT_THROW(
          handler.addUnicastRoute(
              client, makeUnicastRoute(prefix4, nhop4, clientAdmin)),
          FbossError);
      EXPECT_THROW(
          handler.addUnicastRoute(
              client, makeUnicastRoute(prefix6, nhop6, clientAdmin)),
          FbossError);
    } else {
      handler.addUnicastRoute(
          client, makeUnicastRoute(prefix4, nhop4, clientAdmin));
      handler.addUnicastRoute(
          client, makeUnicastRoute(prefix6, nhop6, clientAdmin));
    }
  };

  addRoutesForClient(prefixA4, prefixA6, bgpClient, bgpClientAdmin);
  addRoutesForClient(prefixB4, prefixB6, openrClient, openrClientAdmin);

  auto verifyPrefixesPresent = [&](const auto& prefix4,
                                   const auto& prefix6,
                                   AdminDistance distance) {
    if (this->isFabric()) {
      return;
    }
    auto state = this->sw_->getState();
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefix4), state);
    EXPECT_NE(nullptr, rtA4);
    EXPECT_EQ(
        rtA4->getForwardInfo(),
        RouteNextHopEntry(makeResolvedNextHops({{kIntf1, nhop4}}), distance));

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefix6), state);
    EXPECT_NE(nullptr, rtA6);
    EXPECT_EQ(
        rtA6->getForwardInfo(),
        RouteNextHopEntry(makeResolvedNextHops({{kIntf1, nhop6}}), distance));
  };
  verifyPrefixesPresent(prefixA4, prefixA6, AdminDistance::EBGP);
  verifyPrefixesPresent(prefixB4, prefixB6, AdminDistance::OPENR);

  auto verifyPrefixesRemoved = [&](const auto& prefix4, const auto& prefix6) {
    auto state = this->sw_->getState();
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefix4), state);
    EXPECT_EQ(nullptr, rtA4);
    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefix6), state);
    EXPECT_EQ(nullptr, rtA6);
  };

  // Call syncFib for BGP. Remove all BGP routes and add some new routes
  auto newBgpRoutes = std::make_unique<std::vector<UnicastRoute>>();
  newBgpRoutes->push_back(
      *makeUnicastRoute(prefixC6, nhop6, bgpClientAdmin).get());
  newBgpRoutes->push_back(
      *makeUnicastRoute(prefixC4, nhop4, bgpClientAdmin).get());
  if (this->isFabric()) {
    EXPECT_THROW(
        handler.syncFib(bgpClient, std::move(newBgpRoutes)), FbossError);
  } else {
    handler.syncFib(bgpClient, std::move(newBgpRoutes));
  }

  // verify that old BGP prefixes are removed
  verifyPrefixesRemoved(prefixA4, prefixA6);
  // verify that OPENR prefixes exist
  verifyPrefixesPresent(prefixB4, prefixB6, AdminDistance::OPENR);
  // verify new BGP prefixes are added
  verifyPrefixesPresent(prefixC4, prefixC6, AdminDistance::EBGP);

  // Call syncFib for OPENR. Remove all OPENR routes and add some new routes
  auto newOpenrRoutes = std::make_unique<std::vector<UnicastRoute>>();
  newOpenrRoutes->push_back(
      *makeUnicastRoute(prefixD4, nhop4, openrClientAdmin).get());
  newOpenrRoutes->push_back(
      *makeUnicastRoute(prefixD6, nhop6, openrClientAdmin).get());
  if (this->isFabric()) {
    EXPECT_THROW(
        handler.syncFib(openrClient, std::move(newOpenrRoutes)), FbossError);
  } else {
    handler.syncFib(openrClient, std::move(newOpenrRoutes));
  }

  // verify that old OPENR prefixes are removed
  verifyPrefixesRemoved(prefixB4, prefixB6);
  // verify that new OPENR prefixes are added
  verifyPrefixesPresent(prefixD4, prefixD6, AdminDistance::OPENR);

  // Add back BGP and OPENR routes
  addRoutesForClient(prefixA4, prefixA6, bgpClient, bgpClientAdmin);
  addRoutesForClient(prefixB4, prefixB6, openrClient, openrClientAdmin);

  // verify routes added
  verifyPrefixesPresent(prefixA4, prefixA6, AdminDistance::EBGP);
  verifyPrefixesPresent(prefixB4, prefixB6, AdminDistance::OPENR);
}

TYPED_TEST(ThriftTestAllSwitchTypes, getVlanAddresses) {
  ThriftHandler handler(this->sw_);
  using Addresses = std::vector<facebook::network::thrift::Address>;
  using BinaryAddresses = std::vector<facebook::network::thrift::BinaryAddress>;
  auto constexpr kVlan = 1;
  auto constexpr kVlanName = "Vlan1";
  if (this->isNpu()) {
    {
      Addresses addrs;
      handler.getVlanAddresses(addrs, kVlan);
      EXPECT_GT(addrs.size(), 0);
    }
    {
      Addresses addrs;
      handler.getVlanAddressesByName(
          addrs, std::make_unique<std::string>(kVlanName));
      EXPECT_GT(addrs.size(), 0);
    }
    {
      BinaryAddresses addrs;
      handler.getVlanBinaryAddresses(addrs, kVlan);
      EXPECT_GT(addrs.size(), 0);
    }
    {
      BinaryAddresses addrs;
      handler.getVlanBinaryAddressesByName(
          addrs, std::make_unique<std::string>(kVlanName));
      EXPECT_GT(addrs.size(), 0);
    }
  } else {
    {
      Addresses addrs;
      EXPECT_THROW(handler.getVlanAddresses(addrs, 1), FbossError);
      EXPECT_THROW(
          handler.getVlanAddressesByName(
              addrs, std::make_unique<std::string>(kVlanName)),
          FbossError);
    }
    {
      BinaryAddresses addrs;
      EXPECT_THROW(handler.getVlanBinaryAddresses(addrs, 1), FbossError);
      EXPECT_THROW(
          handler.getVlanBinaryAddressesByName(
              addrs, std::make_unique<std::string>(kVlanName)),
          FbossError);
    }
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getAllInterfaces) {
  ThriftHandler handler(this->sw_);
  std::map<int32_t, InterfaceDetail> intfs;
  handler.getAllInterfaces(intfs);
  if (this->isFabric()) {
    EXPECT_TRUE(intfs.empty());
  } else {
    EXPECT_FALSE(intfs.empty());
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getInterfaceList) {
  ThriftHandler handler(this->sw_);
  std::vector<std::string> intfs;
  handler.getInterfaceList(intfs);
  if (this->isFabric()) {
    EXPECT_TRUE(intfs.empty());
  } else {
    EXPECT_FALSE(intfs.empty());
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getInterfaceDetail) {
  ThriftHandler handler(this->sw_);
  InterfaceDetail intfDetail;
  auto intfId = 1;
  if (this->isFabric()) {
    EXPECT_THROW(handler.getInterfaceDetail(intfDetail, intfId), FbossError);
  } else {
    handler.getInterfaceDetail(intfDetail, intfId);
    EXPECT_EQ(*intfDetail.interfaceId(), intfId);
  }
}

TYPED_TEST(ThriftTestAllSwitchTypes, getAggregatePorts) {
  if (this->isFabric()) {
    // no agg ports on fabric
    GTEST_SKIP();
  }
  auto switchIdAndType = this->getSwitchIdAndType();
  ThriftHandler handler(this->sw_);
  auto startState = this->sw_->getState();

  auto config = testConfigA(switchIdAndType.second);
  config.aggregatePorts()->resize(2);
  *config.aggregatePorts()[0].key() = 55;
  *config.aggregatePorts()[0].name() = "lag55";
  *config.aggregatePorts()[0].description() = "upwards facing link-bundle";
  setAggregatePortMemberIDs(
      *config.aggregatePorts()[0].memberPorts(),
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  *config.aggregatePorts()[1].key() = 155;
  *config.aggregatePorts()[1].name() = "lag155";
  *config.aggregatePorts()[1].description() = "downwards facing link-bundle";
  config.aggregatePorts()[1].memberPorts()->resize(10);
  setAggregatePortMemberIDs(
      *config.aggregatePorts()[1].memberPorts(),
      {11, 12, 13, 14, 15, 16, 17, 18, 19, 20});
  this->sw_->applyConfig("add agg ports", config);
  std::vector<AggregatePortThrift> aggPorts;
  handler.getAggregatePortTable(aggPorts);
  EXPECT_EQ(aggPorts.size(), 2);
  EXPECT_EQ(*aggPorts[0].key(), 55);
  EXPECT_EQ(*aggPorts[1].key(), 155);
  EXPECT_EQ(
      getAggregatePortMemberIDs(*aggPorts[0].memberPorts()),
      getAggregatePortMemberIDs(*config.aggregatePorts()[0].memberPorts()));
  EXPECT_EQ(
      getAggregatePortMemberIDs(*aggPorts[1].memberPorts()),
      getAggregatePortMemberIDs(*config.aggregatePorts()[1].memberPorts()));
}

TYPED_TEST(ThriftTestAllSwitchTypes, getSwitchIndicesForInterfaces) {
  ThriftHandler handler(this->sw_);
  std::map<int16_t, std::vector<std::string>> switchIndicesForInterfaces;
  std::vector<std::string> interfaces;
  handler.getInterfaceList(interfaces);
  if (this->isFabric()) {
    EXPECT_TRUE(interfaces.empty());
    return;
  }
  // Remove dummy recycle port interface if switch type is VOQ
  if (this->isVoq()) {
    interfaces.erase(interfaces.begin());
  }
  handler.getSwitchIndicesForInterfaces(
      switchIndicesForInterfaces,
      std::make_unique<std::vector<std::string>>(interfaces));
  EXPECT_EQ(switchIndicesForInterfaces.size(), 1);
  EXPECT_EQ(switchIndicesForInterfaces.begin()->first, 0);
  auto fetchedInterfaces = switchIndicesForInterfaces.begin()->second;
  for (auto i = 0; i < interfaces.size(); i++) {
    EXPECT_EQ(interfaces[i], fetchedInterfaces[i]);
  }
}

TEST_F(ThriftTest, getAndSetMacAddrsToBlock) {
  ThriftHandler handler(sw_);

  auto blockListVerify =
      [&handler](
          std::vector<std::pair<VlanID, folly::MacAddress>> macAddrsToBlock) {
        auto cfgMacAddrsToBlock =
            std::make_unique<std::vector<cfg::MacAndVlan>>();

        for (const auto& [vlanID, macAddress] : macAddrsToBlock) {
          cfg::MacAndVlan macAndVlan;
          macAndVlan.vlanID() = vlanID;
          macAndVlan.macAddress() = macAddress.toString();
          cfgMacAddrsToBlock->emplace_back(macAndVlan);
        }
        auto expectedCfgMacAddrsToBlock = *cfgMacAddrsToBlock;
        handler.setMacAddrsToBlock(std::move(cfgMacAddrsToBlock));
        waitForStateUpdates(handler.getSw());

        auto gotMacAddrsToBlock =
            utility::getFirstNodeIf(
                handler.getSw()->getState()->getSwitchSettings())
                ->getMacAddrsToBlock_DEPRECATED();
        EXPECT_EQ(macAddrsToBlock, gotMacAddrsToBlock);

        std::vector<cfg::MacAndVlan> gotMacAddrsToBlockViaThrift;
        handler.getMacAddrsToBlock(gotMacAddrsToBlockViaThrift);
        EXPECT_EQ(gotMacAddrsToBlockViaThrift, expectedCfgMacAddrsToBlock);
      };

  // set blockneighbor1
  blockListVerify({{VlanID(2000), folly::MacAddress("00:11:22:33:44:55")}});

  // set blockneighbor1, blockNeighbor2
  blockListVerify(
      {{VlanID(2000), folly::MacAddress("00:11:22:33:44:55")},
       {VlanID(2000), folly::MacAddress("00:11:22:33:44:66")}});

  // set blockNeighbor2
  blockListVerify({{VlanID(2000), folly::MacAddress("00:11:22:33:44:66")}});

  // set null list (clears block list)
  std::vector<cfg::MacAndVlan> macAddrsToBlock;
  handler.setMacAddrsToBlock({});
  waitForStateUpdates(sw_);
  EXPECT_EQ(
      0,
      utility::getFirstNodeIf(sw_->getState()->getSwitchSettings())
          ->getMacAddrsToBlock_DEPRECATED()
          .size());
  handler.getMacAddrsToBlock(macAddrsToBlock);
  EXPECT_TRUE(macAddrsToBlock.empty());

  // set empty list (clears block list)
  auto macAddrsToBlock2 = std::make_unique<std::vector<cfg::MacAndVlan>>();
  handler.setMacAddrsToBlock(std::move(macAddrsToBlock2));
  waitForStateUpdates(sw_);
  EXPECT_EQ(
      0,
      utility::getFirstNodeIf(sw_->getState()->getSwitchSettings())
          ->getMacAddrsToBlock_DEPRECATED()
          .size());
  handler.getMacAddrsToBlock(macAddrsToBlock);
  EXPECT_TRUE(macAddrsToBlock.empty());

  auto invalidMacAddrToBlock =
      "twshared12345.06.abc7"; // only MACs are supported
  auto invalidMacAddrsToBlock =
      std::make_unique<std::vector<cfg::MacAndVlan>>();
  cfg::MacAndVlan macAndVlan;
  macAndVlan.vlanID() = 2000;
  macAndVlan.macAddress() = invalidMacAddrToBlock;
  invalidMacAddrsToBlock->emplace_back(macAndVlan);
  EXPECT_THROW(
      handler.setMacAddrsToBlock(std::move(invalidMacAddrsToBlock)),
      FbossError);
  handler.getMacAddrsToBlock(macAddrsToBlock);
  EXPECT_TRUE(macAddrsToBlock.empty());
}

TEST_F(ThriftTest, setNeighborsToBlockAndMacAddrsToBlock) {
  ThriftHandler handler(sw_);

  auto getNeighborsToBlock = []() {
    auto neighborsToBlock = std::make_unique<std::vector<cfg::Neighbor>>();
    cfg::Neighbor neighbor;
    neighbor.vlanID() = 2000;
    neighbor.ipAddress() = "2401:db00:2110:3001::0003";
    neighborsToBlock->emplace_back(neighbor);
    return neighborsToBlock;
  };

  auto getMacAddrsToBlock = []() {
    auto macAddrsToBlock = std::make_unique<std::vector<cfg::MacAndVlan>>();
    cfg::MacAndVlan macAndVlan;
    macAndVlan.vlanID() = 2000;
    macAndVlan.macAddress() = "00:11:22:33:44:55";
    macAddrsToBlock->emplace_back(macAndVlan);
    return macAddrsToBlock;
  };

  // set neighborsToBlock and then set macAddrsToBlock: expect FAIL
  handler.setNeighborsToBlock(getNeighborsToBlock());
  waitForStateUpdates(handler.getSw());
  EXPECT_THROW(handler.setMacAddrsToBlock(getMacAddrsToBlock()), FbossError);
  waitForStateUpdates(handler.getSw());

  // clear neighborsToBlock and then set macAddrsToBlock: expect PASS
  handler.setNeighborsToBlock({});
  waitForStateUpdates(handler.getSw());
  handler.setMacAddrsToBlock(getMacAddrsToBlock());
  waitForStateUpdates(handler.getSw());

  // macAddrsToBlock already set, now set neighborsToBlock: expect FAIL
  EXPECT_THROW(handler.setNeighborsToBlock(getNeighborsToBlock()), FbossError);
  waitForStateUpdates(handler.getSw());
}

// Test for the ThriftHandler::syncFib method
TEST_F(ThriftTest, syncFib) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto randomClient = 500;
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto staticClient = static_cast<int16_t>(ClientID::STATIC_ROUTE);
  auto randomClientAdmin = sw_->clientIdToAdminDistance(randomClient);
  auto bgpAdmin = sw_->clientIdToAdminDistance(bgpClient);
  auto staticAdmin = sw_->clientIdToAdminDistance(staticClient);
  // STATIC_ROUTE > BGPD > RANDOM_CLIENT
  //
  // Add a few BGP routes
  //

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";
  auto cli2_nhop4 = "10.0.0.22";
  auto cli2_nhop6 = "2401:db00:2110:3001::0022";
  auto cli3_nhop6 = "2401:db00:2110:3001::0033";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA6, cli1_nhop6, randomClientAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  auto prefixB4 = "7.2.0.0/16";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixB4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixB4, cli2_nhop4, bgpAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  // and staticClient
  auto prefixC6 = "aaaa:3::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixC6, cli2_nhop6, bgpAdmin));
  handler.addUnicastRoute(
      staticClient, makeUnicastRoute(prefixC6, cli3_nhop6, staticAdmin));

  // These routes will not be used until fibSync happens.
  auto prefixD4 = "7.4.0.0/16";
  auto prefixD6 = "aaaa:4::0/64";

  //
  // Test the state of things before calling syncFib
  //

  // Make sure all the static and link-local routes are there
  auto ensureConfigRoutes = [this, rid]() {
    auto state = sw_->getState();
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV4>(
            rid, IPAddress::createNetwork("10.0.0.0/24"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV4>(
            rid, IPAddress::createNetwork("192.168.0.0/24"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV6>(
            rid, IPAddress::createNetwork("2401:db00:2110:3001::/64"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV6>(
            rid, IPAddress::createNetwork("fe80::/64"), state));
  };
  auto kIntf1 = InterfaceID(1);
  ensureConfigRoutes();
  // Make sure the lowest adming distance route is installed in FIB routes are
  // there.

  {
    auto state = sw_->getState();
    // Only random client routes
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixA4), state);
    EXPECT_NE(nullptr, rtA4);
    EXPECT_EQ(
        rtA4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixA6), state);
    EXPECT_NE(nullptr, rtA6);
    EXPECT_EQ(
        rtA6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // Random client and BGP routes - bgp should win
    auto rtB4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixB4), state);
    EXPECT_NE(nullptr, rtB4);
    EXPECT_EQ(
        rtB4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli2_nhop4}}), AdminDistance::EBGP));
    // Random client, bgp, static routes. Static shouold win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::STATIC_ROUTE));
    auto [v4Routes, v6Routes] = getRouteCount(state);
    EXPECT_EQ(
        8, v4Routes); // 5 intf routes + 2 routes from above + 1 default routes
    EXPECT_EQ(
        6, v6Routes); // 2 intf routes + 2 routes from above + 1 link local
                      // + 1 default route
    // Unistalled routes. These should not be found
    EXPECT_EQ(
        nullptr,
        findRoute<folly::IPAddressV4>(
            rid, IPAddress::createNetwork(prefixD4), state));
    EXPECT_EQ(
        nullptr,
        findRoute<folly::IPAddressV6>(
            rid, IPAddress::createNetwork(prefixD6), state));
  }
  //
  // Now use syncFib to remove all the routes for randomClient and add
  // some new ones
  // Statics, link-locals, and clients bgp and static should remain unchanged.
  //

  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 =
      *makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin).get();
  UnicastRoute nr2 =
      *makeUnicastRoute(prefixD6, cli1_nhop6, randomClientAdmin).get();
  UnicastRoute nr3 =
      *makeUnicastRoute(prefixD4, cli1_nhop4, randomClientAdmin).get();
  newRoutes->push_back(nr1);
  newRoutes->push_back(nr2);
  newRoutes->push_back(nr3);
  handler.syncFib(randomClient, std::move(newRoutes));

  //
  // Test the state of things after syncFib
  //
  {
    // Make sure all the static and link-local routes are still there
    auto state = sw_->getState();
    ensureConfigRoutes();
    // Only random client routes from before are gone, since we did not syncFib
    // with them
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixA4), state);
    EXPECT_EQ(nullptr, rtA4);

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixA6), state);
    EXPECT_EQ(nullptr, rtA6);
    // random client and bgp routes. Bgp should continue to win
    auto rtB4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixB4), state);
    EXPECT_TRUE(rtB4->getForwardInfo().isSame(RouteNextHopEntry(
        makeResolvedNextHops({{InterfaceID(1), cli2_nhop4}}),
        AdminDistance::EBGP)));

    // Random client, bgp, static routes. Static should win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::STATIC_ROUTE));
    // D6 and D4 should now be found and resolved by random client nhops
    auto rtD4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixD4), state);
    EXPECT_NE(nullptr, rtD4);
    EXPECT_EQ(
        rtD4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    auto rtD6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixD6), state);
    EXPECT_NE(nullptr, rtD6);
    EXPECT_EQ(
        rtD6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // A4, A6 removed, D4, D6 added. Count should remain same
    auto [v4Routes, v6Routes] = getRouteCount(state);
    EXPECT_EQ(8, v4Routes);
    EXPECT_EQ(6, v6Routes);
  }
}

// Test for the ThriftHandler::add/del Unicast routes methods
// This test is a replica of syncFib test from above, except that
// when adding, deleting routes for a client it uses add, del
// UnicastRoute APIs instead of syncFib
TEST_F(ThriftTest, addDelUnicastRoutes) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto randomClient = 500;
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto staticClient = static_cast<int16_t>(ClientID::STATIC_ROUTE);
  auto randomClientAdmin = sw_->clientIdToAdminDistance(randomClient);
  auto bgpAdmin = sw_->clientIdToAdminDistance(bgpClient);
  auto staticAdmin = sw_->clientIdToAdminDistance(staticClient);
  // STATIC_ROUTE > BGPD > RANDOM_CLIENT
  //
  // Add a few BGP routes
  //

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";
  auto cli2_nhop4 = "10.0.0.22";
  auto cli2_nhop6 = "2401:db00:2110:3001::0022";
  auto cli3_nhop6 = "2401:db00:2110:3001::0033";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA6, cli1_nhop6, randomClientAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  auto prefixB4 = "7.2.0.0/16";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixB4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixB4, cli2_nhop4, bgpAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  // and staticClient
  auto prefixC6 = "aaaa:3::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixC6, cli2_nhop6, bgpAdmin));
  handler.addUnicastRoute(
      staticClient, makeUnicastRoute(prefixC6, cli3_nhop6, staticAdmin));

  // These routes will not be used until fibSync happens.
  auto prefixD4 = "7.4.0.0/16";
  auto prefixD6 = "aaaa:4::0/64";

  //
  // Test the state of things before calling syncFib
  //

  // Make sure all the static and link-local routes are there
  auto ensureConfigRoutes = [this, rid]() {
    auto state = sw_->getState();
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV4>(

            rid, IPAddress::createNetwork("10.0.0.0/24"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV4>(

            rid, IPAddress::createNetwork("192.168.0.0/24"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV6>(

            rid, IPAddress::createNetwork("2401:db00:2110:3001::/64"), state));
    EXPECT_NE(
        nullptr,
        findRoute<folly::IPAddressV6>(

            rid, IPAddress::createNetwork("fe80::/64"), state));
  };
  auto kIntf1 = InterfaceID(1);
  ensureConfigRoutes();
  // Make sure the lowest admin distance route is installed in FIB routes are
  // there.

  {
    auto state = sw_->getState();
    // Only random client routes
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixA4), state);
    EXPECT_NE(nullptr, rtA4);
    EXPECT_EQ(
        rtA4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixA6), state);
    EXPECT_NE(nullptr, rtA6);
    EXPECT_EQ(
        rtA6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // Random client and BGP routes - bgp should win
    auto rtB4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixB4), state);
    EXPECT_NE(nullptr, rtB4);
    EXPECT_EQ(
        rtB4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli2_nhop4}}), AdminDistance::EBGP));
    // Random client, bgp, static routes. Static should win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::STATIC_ROUTE));
    auto [v4Routes, v6Routes] = getRouteCount(state);
    EXPECT_EQ(
        8, v4Routes); // 5 intf routes + 2 routes from above + 1 default routes
    EXPECT_EQ(
        6, v6Routes); // 2 intf routes + 2 routes from above + 1 link local
                      // + 1 default route
    // Unistalled routes. These should not be found
    EXPECT_EQ(
        nullptr,
        findRoute<folly::IPAddressV4>(
            rid, IPAddress::createNetwork(prefixD4), state));
    EXPECT_EQ(
        nullptr,
        findRoute<folly::IPAddressV6>(
            rid, IPAddress::createNetwork(prefixD6), state));
  }
  //
  // Now use deleteUnicastRoute to remove all the routes for randomClient and
  // add some new ones Statics, link-locals, and clients bgp and static should
  // remain unchanged.
  //
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork(prefixA4)),
      ipPrefix(IPAddress::createNetwork(prefixA6)),
      ipPrefix(IPAddress::createNetwork(prefixB4)),
  };
  handler.deleteUnicastRoutes(
      randomClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));
  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 =
      *makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin).get();
  UnicastRoute nr2 =
      *makeUnicastRoute(prefixD6, cli1_nhop6, randomClientAdmin).get();
  UnicastRoute nr3 =
      *makeUnicastRoute(prefixD4, cli1_nhop4, randomClientAdmin).get();
  newRoutes->push_back(nr1);
  newRoutes->push_back(nr2);
  newRoutes->push_back(nr3);
  handler.addUnicastRoutes(randomClient, std::move(newRoutes));

  //
  // Test the state of things after syncFib
  //
  {
    // Make sure all the static and link-local routes are still there
    auto state = sw_->getState();
    ensureConfigRoutes();
    // Only random client routes from before are gone, since we did not syncFib
    // with them
    auto rtA4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixA4), state);
    EXPECT_EQ(nullptr, rtA4);

    auto rtA6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixA6), state);
    EXPECT_EQ(nullptr, rtA6);
    // random client and bgp routes. Bgp should continue to win
    auto rtB4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixB4), state);
    EXPECT_TRUE(rtB4->getForwardInfo().isSame(RouteNextHopEntry(
        makeResolvedNextHops({{InterfaceID(1), cli2_nhop4}}),
        AdminDistance::EBGP)));

    // Random client, bgp, static routes. Static should win
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), state);
    EXPECT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli3_nhop6}}),
            AdminDistance::STATIC_ROUTE));
    // D6 and D4 should now be found and resolved by random client nhops
    auto rtD4 = findRoute<folly::IPAddressV4>(
        rid, IPAddress::createNetwork(prefixD4), state);
    EXPECT_NE(nullptr, rtD4);
    EXPECT_EQ(
        rtD4->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop4}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    auto rtD6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixD6), state);
    EXPECT_NE(nullptr, rtD6);
    EXPECT_EQ(
        rtD6->getForwardInfo(),
        RouteNextHopEntry(
            makeResolvedNextHops({{kIntf1, cli1_nhop6}}),
            AdminDistance::MAX_ADMIN_DISTANCE));
    // A4, A6 removed, D4, D6 added. Count should remain same
    auto [v4Routes, v6Routes] = getRouteCount(state);
    EXPECT_EQ(8, v4Routes);
    EXPECT_EQ(6, v6Routes);
  }
}

TEST_F(ThriftTest, delUnicastRoutes) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto randomClient = 500;
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto staticClient = static_cast<int16_t>(ClientID::STATIC_ROUTE);
  auto randomClientAdmin = sw_->clientIdToAdminDistance(randomClient);
  auto bgpAdmin = sw_->clientIdToAdminDistance(bgpClient);
  auto staticAdmin = sw_->clientIdToAdminDistance(staticClient);
  // STATIC_ROUTE > BGPD > RANDOM_CLIENT
  //
  // Add a few BGP routes
  //

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";
  auto cli2_nhop4 = "10.0.0.22";
  auto cli2_nhop6 = "2401:db00:2110:3001::0022";
  auto cli3_nhop6 = "2401:db00:2110:3001::0033";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA6, cli1_nhop6, randomClientAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  auto prefixB4 = "7.2.0.0/16";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixB4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixB4, cli2_nhop4, bgpAdmin));

  // This route will include nexthops from clients randomClient and bgpClient
  // and staticClient
  auto prefixC6 = "aaaa:3::0/64";
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin));
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixC6, cli2_nhop6, bgpAdmin));
  handler.addUnicastRoute(
      staticClient, makeUnicastRoute(prefixC6, cli3_nhop6, staticAdmin));

  auto assertRoute = [rid, prefixC6, this](
                         bool expectPresent,
                         const std::string& nhop,
                         AdminDistance distance) {
    auto kIntf1 = InterfaceID(1);
    auto rtC6 = findRoute<folly::IPAddressV6>(
        rid, IPAddress::createNetwork(prefixC6), sw_->getState());
    if (!expectPresent) {
      EXPECT_EQ(nullptr, rtC6);
      return;
    }
    ASSERT_NE(nullptr, rtC6);
    EXPECT_EQ(
        rtC6->getForwardInfo(),
        RouteNextHopEntry(makeResolvedNextHops({{kIntf1, nhop}}), distance));
  };
  // Random client, bgp, static routes. Static should win
  assertRoute(true, cli3_nhop6, AdminDistance::STATIC_ROUTE);
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork(prefixC6)),
  };
  // Now delete prefixC6 for Static client. BGP should win
  handler.deleteUnicastRoutes(
      staticClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));
  // Random client, bgp, routes. BGP should win
  assertRoute(true, cli2_nhop6, AdminDistance::EBGP);
  // Now delete prefixC6 for BGP client. random client should win
  // For good measure - use the single route delete API here
  handler.deleteUnicastRoute(
      bgpClient,
      std::make_unique<IpPrefix>(ipPrefix(IPAddress::createNetwork(prefixC6))));
  // Random client routes only
  assertRoute(true, cli1_nhop6, AdminDistance::MAX_ADMIN_DISTANCE);
  // Now delete prefixC6 for random client. Route should be dropped now
  handler.deleteUnicastRoutes(
      randomClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));
  // Random client, bgp, routes. BGP should win
  assertRoute(false, "none", AdminDistance::EBGP);
  // Add routes back and see that lowest admin distance route comes in
  // again
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixC6, cli1_nhop6, randomClientAdmin));
  // Random client routes only
  assertRoute(true, cli1_nhop6, AdminDistance::MAX_ADMIN_DISTANCE);
  handler.addUnicastRoute(
      bgpClient, makeUnicastRoute(prefixC6, cli2_nhop6, bgpAdmin));
  // Random client, bgp, routes. BGP should win
  assertRoute(true, cli2_nhop6, AdminDistance::EBGP);
  handler.addUnicastRoute(
      staticClient, makeUnicastRoute(prefixC6, cli3_nhop6, staticAdmin));
  // Random client, bgp, static routes. Static should win
  assertRoute(true, cli3_nhop6, AdminDistance::STATIC_ROUTE);
}

TEST_F(ThriftTest, syncFibIsHwProtected) {
  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);
  auto addRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 =
      *makeUnicastRoute("aaaa::/64", "2401:db00:2110:3001::1").get();
  addRoutes->push_back(nr1);
  EXPECT_STATE_UPDATE(sw_);
  handler.addUnicastRoutes(10, std::move(addRoutes));
  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr2 = *makeUnicastRoute("bbbb::/64", "42::42").get();
  newRoutes->push_back(nr2);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(sw_, stateChangedImpl(_))
      .Times(::testing::AtLeast(1))
      .WillOnce(Return(sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.syncFib(10, std::move(newRoutes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.vrf2failedAddUpdatePrefixes()->size(), 1);
          auto itr = fibError.vrf2failedAddUpdatePrefixes()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          itr = fibError.vrf2failedDeletePrefixes()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, addUnicastRoutesIsHwProtected) {
  ThriftHandler handler(sw_);
  auto newRoutes = std::make_unique<std::vector<UnicastRoute>>();
  UnicastRoute nr1 = *makeUnicastRoute("aaaa::/64", "42::42").get();
  newRoutes->push_back(nr1);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(sw_, stateChangedImpl(_)).WillOnce(Return(sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.addUnicastRoutes(10, std::move(newRoutes));

        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.vrf2failedAddUpdatePrefixes()->size(), 1);
          auto itr = fibError.vrf2failedAddUpdatePrefixes()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, getRouteTable) {
  ThriftHandler handler(sw_);
  auto [v4Routes, v6Routes] = getRouteCount(sw_->getState());
  std::vector<UnicastRoute> routeTable;
  handler.getRouteTable(routeTable);
  // 7 intf routes + 2 default routes + 1 link local route
  EXPECT_EQ(10, v4Routes + v6Routes);
  EXPECT_EQ(10, routeTable.size());
}

TEST_F(ThriftTest, getRouteDetails) {
  ThriftHandler handler(sw_);
  auto [v4Routes, v6Routes] = getRouteCount(sw_->getState());
  std::vector<RouteDetails> routeDetails;
  handler.getRouteTableDetails(routeDetails);
  // 7 intf routes + 2 default routes + 1 link local route
  EXPECT_EQ(10, v4Routes + v6Routes);
  EXPECT_EQ(10, routeDetails.size());
}

TEST_F(ThriftTest, getRouteTableByClient) {
  ThriftHandler handler(sw_);
  std::vector<UnicastRoute> routeTable;
  handler.getRouteTableByClient(
      routeTable, static_cast<int16_t>(ClientID::INTERFACE_ROUTE));
  // 6 intf routes + 2 default routes + 1 link local route
  EXPECT_EQ(7, routeTable.size());
}
std::unique_ptr<MplsRoute> makeMplsRoute(
    int32_t mplsLabel,
    std::string nxtHop,
    AdminDistance distance = AdminDistance::MAX_ADMIN_DISTANCE) {
  auto nr = std::make_unique<MplsRoute>();
  nr->topLabel() = mplsLabel;
  NextHopThrift nh;
  MplsAction mplsAction;
  mplsAction.action() = MplsActionCode::POP_AND_LOOKUP;
  nh.address() = toBinaryAddress(IPAddress(nxtHop));
  nh.mplsAction() = mplsAction;
  nr->nextHops()->push_back(nh);
  nr->adminDistance() = distance;
  return nr;
}

TEST_F(ThriftTest, syncMplsFibIsHwProtected) {
  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);
  auto newRoutes = std::make_unique<std::vector<MplsRoute>>();
  MplsRoute nr1 = *makeMplsRoute(101, "10.0.0.2").get();
  newRoutes->push_back(nr1);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(sw_, stateChangedImpl(_))
      .WillRepeatedly(Return(sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.syncMplsFib(10, std::move(newRoutes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.failedAddUpdateMplsLabels()->size(), 1);
          EXPECT_EQ(*fibError.failedAddUpdateMplsLabels()->begin(), 101);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, addMplsRoutesIsHwProtected) {
  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);
  auto newRoutes = std::make_unique<std::vector<MplsRoute>>();
  MplsRoute nr1 = *makeMplsRoute(101, "10.0.0.2").get();
  newRoutes->push_back(nr1);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(sw_, stateChangedImpl(_))
      .WillRepeatedly(Return(sw_->getState()));
  EXPECT_THROW(
      {
        try {
          handler.addMplsRoutes(10, std::move(newRoutes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.failedAddUpdateMplsLabels()->size(), 1);
          EXPECT_EQ(*fibError.failedAddUpdateMplsLabels()->begin(), 101);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, hwUpdateErrorAfterPartialUpdate) {
  ThriftHandler handler(sw_);
  std::vector<UnicastRoute>();
  UnicastRoute nr1 =
      *makeUnicastRoute("aaaa::/64", "2401:db00:2110:3001::1").get();
  std::vector<UnicastRoute> routes;
  routes.push_back(nr1);
  EXPECT_STATE_UPDATE_TIMES(sw_, 2);
  handler.addUnicastRoutes(
      10, std::make_unique<std::vector<UnicastRoute>>(routes));
  auto oneRouteAddedState = sw_->getState();
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork("aaaa::/64")),
  };
  // Delete added route so we revert back to starting state
  handler.deleteUnicastRoutes(
      10, std::make_unique<std::vector<IpPrefix>>(delRoutes));
  // Now try to add 2 routes, have the HwSwitch fail after adding one route
  UnicastRoute nr2 =
      *makeUnicastRoute("bbbb::/64", "2401:db00:2110:3001::1").get();
  routes.push_back(nr2);
  // Fail HW update by returning one route added state.
  EXPECT_HW_CALL(sw_, stateChangedImpl(_))
      .Times(::testing::AtLeast(1))
      .WillOnce(Return(oneRouteAddedState));
  EXPECT_THROW(
      {
        try {
          handler.addUnicastRoutes(
              10, std::make_unique<std::vector<UnicastRoute>>(routes));
        } catch (const FbossFibUpdateError& fibError) {
          EXPECT_EQ(fibError.vrf2failedAddUpdatePrefixes()->size(), 1);
          auto itr = fibError.vrf2failedAddUpdatePrefixes()->find(0);
          EXPECT_EQ(itr->second.size(), 1);
          itr = fibError.vrf2failedDeletePrefixes()->find(0);
          EXPECT_EQ(itr->second.size(), 0);
          throw;
        }
      },
      FbossFibUpdateError);
}

TEST_F(ThriftTest, routeUpdatesWithConcurrentReads) {
  auto thriftHgridRoutes =
      utility::HgridDuRouteScaleGenerator(sw_->getState(), 100000)
          .getThriftRoutes()[0];
  auto thriftRswRoutes = utility::RSWRouteScaleGenerator(sw_->getState(), 10000)
                             .getThriftRoutes()[0];
  std::atomic<bool> done{false};

  ThriftHandler handler(sw_);
  std::thread routeReads([&handler, &done]() {
    while (!done) {
      std::vector<RouteDetails> details;
      handler.getRouteTableDetails(details);
      UnicastRoute route;
      handler.getIpRoute(
          route,
          std::make_unique<facebook::network::thrift::Address>(
              facebook::network::toAddress(folly::IPAddress("2001::"))),
          RouterID(0));
    }
  });
  handler.addUnicastRoutes(
      10, std::make_unique<std::vector<UnicastRoute>>(thriftHgridRoutes));
  handler.addUnicastRoutes(
      11, std::make_unique<std::vector<UnicastRoute>>(thriftRswRoutes));
  done = true;
  routeReads.join();
}

TEST_F(ThriftTest, UnicastRoutesWithClassID) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto randomClient = 500;
  auto randomClientAdmin = sw_->clientIdToAdminDistance(randomClient);
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";

  std::optional<cfg::AclLookupClass> classID1(
      cfg::AclLookupClass::DST_CLASS_L3_DPR);
  std::optional<cfg::AclLookupClass> classID2(
      cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);

  // Add BGP routes with class ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(
          prefixA4, cli1_nhop4, bgpClientAdmin, std::nullopt, classID1));
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(
          prefixA6, cli1_nhop6, bgpClientAdmin, std::nullopt, classID2));

  // Add random client routes with no class ID
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA6, cli1_nhop6, randomClientAdmin));

  // BGP route should get selected and class ID set
  auto state = sw_->getState();
  auto rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getClassID(), classID1);
  EXPECT_EQ(rtA4->getForwardInfo().getClassID(), classID1);
  if (auto classID = rtA4->getEntryForClient(ClientID::BGPD)->getClassID()) {
    EXPECT_EQ(*classID, classID1);
  }
  EXPECT_TRUE(!(rtA4->getEntryForClient(static_cast<ClientID>(randomClient))
                    ->getClassID()));
  auto rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getClassID(), classID2);
  EXPECT_EQ(rtA6->getForwardInfo().getClassID(), classID2);
  EXPECT_EQ(*(rtA6->getEntryForClient(ClientID::BGPD)->getClassID()), classID2);
  EXPECT_FALSE(rtA6->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->getClassID());

  // delete BGP routes
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork(prefixA4)),
      ipPrefix(IPAddress::createNetwork(prefixA6)),
  };
  XLOG(DBG2) << "Deleting routes";
  handler.deleteUnicastRoutes(
      bgpClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));

  // class IDs should not be active
  state = sw_->getState();
  rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getClassID(), std::nullopt);
  EXPECT_EQ(rtA4->getForwardInfo().getClassID(), std::nullopt);
  rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getClassID(), std::nullopt);
  EXPECT_EQ(rtA6->getForwardInfo().getClassID(), std::nullopt);

  // Add back BGP routes with class ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(
          prefixA4, cli1_nhop4, bgpClientAdmin, std::nullopt, classID1));
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(
          prefixA6, cli1_nhop6, bgpClientAdmin, std::nullopt, classID2));
  state = sw_->getState();
  rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);

  // class id should get set again
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getClassID(), classID1);
  EXPECT_EQ(rtA4->getForwardInfo().getClassID(), classID1);
  EXPECT_EQ(*(rtA4->getEntryForClient(ClientID::BGPD)->getClassID()), classID1);
  EXPECT_FALSE(rtA4->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->getClassID());
  rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getClassID(), classID2);
  EXPECT_EQ(rtA6->getForwardInfo().getClassID(), classID2);
  EXPECT_EQ(*(rtA6->getEntryForClient(ClientID::BGPD)->getClassID()), classID2);
  EXPECT_FALSE(rtA6->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->getClassID());
}

TEST_F(ThriftTest, UnicastRoutesWithCounterID) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto randomClient = 500;
  auto randomClientAdmin = sw_->clientIdToAdminDistance(randomClient);
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";

  std::optional<RouteCounterID> counterID1("route.counter.0");
  std::optional<RouteCounterID> counterID2("route.counter.1");

  // Add BGP routes with counter ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA4, cli1_nhop4, bgpClientAdmin, counterID1));
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA6, cli1_nhop6, bgpClientAdmin, counterID2));

  // Add random client routes with no counter ID
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA4, cli1_nhop4, randomClientAdmin));
  handler.addUnicastRoute(
      randomClient, makeUnicastRoute(prefixA6, cli1_nhop6, randomClientAdmin));

  // BGP route should get selected and counter ID set
  auto state = sw_->getState();
  auto rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getForwardInfo().getCounterID(), counterID1);
  EXPECT_EQ(
      *(rtA4->getEntryForClient(ClientID::BGPD)->getCounterID()), counterID1);
  EXPECT_FALSE(rtA4->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->getCounterID());
  auto rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getForwardInfo().getCounterID(), counterID2);
  EXPECT_EQ(
      *(rtA6->getEntryForClient(ClientID::BGPD)->getCounterID()), counterID2);
  EXPECT_FALSE(rtA6->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->getCounterID());

  // delete BGP routes
  std::vector<IpPrefix> delRoutes = {
      ipPrefix(IPAddress::createNetwork(prefixA4)),
      ipPrefix(IPAddress::createNetwork(prefixA6)),
  };
  XLOG(DBG2) << "Deleting routes";
  handler.deleteUnicastRoutes(
      bgpClient, std::make_unique<std::vector<IpPrefix>>(delRoutes));

  // counter IDs should not be active
  state = sw_->getState();
  rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getForwardInfo().getCounterID(), std::nullopt);
  rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getForwardInfo().getCounterID(), std::nullopt);

  // Add back BGP routes with counter ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA4, cli1_nhop4, bgpClientAdmin, counterID1));
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA6, cli1_nhop6, bgpClientAdmin, counterID2));
  state = sw_->getState();
  rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);

  // counter id should get set again
  EXPECT_NE(nullptr, rtA4);
  EXPECT_EQ(rtA4->getForwardInfo().getCounterID(), counterID1);
  EXPECT_EQ(
      *(rtA4->getEntryForClient(ClientID::BGPD)->getCounterID()), counterID1);
  EXPECT_FALSE(rtA4->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->getCounterID());
  rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  EXPECT_EQ(rtA6->getForwardInfo().getCounterID(), counterID2);
  EXPECT_EQ(
      *(rtA6->getEntryForClient(ClientID::BGPD)->getCounterID()), counterID2);
  EXPECT_FALSE(rtA6->getEntryForClient(static_cast<ClientID>(randomClient))
                   ->getCounterID());
}

TEST_F(ThriftTest, CounterIDThriftReadTest) {
  RouterID rid = RouterID(0);

  // Create a mock SwSwitch using the config, and wrap it in a ThriftHandler
  ThriftHandler handler(sw_);

  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);

  auto cli1_nhop4 = "10.0.0.11";
  auto cli1_nhop6 = "2401:db00:2110:3001::0011";

  // These routes will include nexthops from client 10 only
  auto prefixA4 = "7.1.0.0/16";
  auto prefixA6 = "aaaa:1::0/64";
  auto prefixB6 = "aaaa:2::0/64";

  std::optional<RouteCounterID> counterID1("route.counter.0");
  std::optional<RouteCounterID> counterID2("route.counter.1");

  // Add BGP routes with counter ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA4, cli1_nhop4, bgpClientAdmin, counterID1));
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA6, cli1_nhop6, bgpClientAdmin, counterID2));
  // This route shares counterID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixB6, cli1_nhop6, bgpClientAdmin, counterID2));

  auto state = sw_->getState();
  auto rtA4 = findRoute<folly::IPAddressV4>(
      rid, IPAddress::createNetwork(prefixA4), state);
  EXPECT_NE(nullptr, rtA4);
  auto rtA6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixA6), state);
  EXPECT_NE(nullptr, rtA6);
  auto rtB6 = findRoute<folly::IPAddressV6>(
      rid, IPAddress::createNetwork(prefixB6), state);
  EXPECT_NE(nullptr, rtB6);

  auto updateCounter = [](auto counterID) {
    auto counter = std::make_unique<MonotonicCounter>(
        *counterID, facebook::fb303::SUM, facebook::fb303::RATE);
    auto now = duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch());
    counter->updateValue(now, 0);
    now = duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch());
    counter->updateValue(now, 64);
  };
  // create and update route counters
  updateCounter(counterID1);
  updateCounter(counterID2);
  CounterCache counters(sw_);
  counters.update();

  std::map<std::string, std::int64_t> routeCounters;
  auto counterIDs = std::make_unique<std::vector<std::string>>();
  auto verifyCounters = [&routeCounters]() {
    EXPECT_EQ(routeCounters.size(), 2);
    for (const auto& counterAndBytes : routeCounters) {
      EXPECT_EQ(counterAndBytes.second, 64);
    }
  };
  handler.getAllRouteCounterBytes(routeCounters);
  verifyCounters();

  routeCounters.clear();
  counterIDs->emplace_back(*counterID1);
  counterIDs->emplace_back(*counterID2);
  handler.getRouteCounterBytes(routeCounters, std::move(counterIDs));
  verifyCounters();

  routeCounters.clear();
  counterIDs = std::make_unique<std::vector<std::string>>();
  // invalid counter should return 0
  counterIDs->emplace_back("invalid");
  handler.getRouteCounterBytes(routeCounters, std::move(counterIDs));
  EXPECT_EQ(routeCounters.size(), 1);
  auto counterBytes = routeCounters.find("invalid");
  EXPECT_NE(counterBytes, routeCounters.end());
  EXPECT_EQ(counterBytes->second, 0);
}

TEST_F(ThriftTest, getRouteTableVerifyCounterID) {
  ThriftHandler handler(sw_);
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);

  auto cli1_nhop6 = "2401:db00:2110:3001::0011";
  auto prefixA6 = "aaaa:1::0/64";
  auto addrA6 = folly::IPAddress("aaaa:1::0");
  std::optional<RouteCounterID> counterID1("route.counter.0");

  // Add BGP routes with counter ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(prefixA6, cli1_nhop6, bgpClientAdmin, counterID1));

  std::vector<UnicastRoute> routeTable;
  bool found = false;
  handler.getRouteTable(routeTable);
  for (const auto& rt : routeTable) {
    if (rt.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*rt.counterID(), *counterID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  routeTable.resize(0);
  found = false;
  handler.getRouteTableByClient(routeTable, bgpClient);
  for (const auto& rt : routeTable) {
    if (rt.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*rt.counterID(), *counterID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  std::vector<RouteDetails> routeDetails;
  found = false;
  handler.getRouteTableDetails(routeDetails);
  for (const auto& route : routeDetails) {
    if (route.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*route.counterID(), *counterID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  UnicastRoute route;
  auto addr = std::make_unique<facebook::network::thrift::Address>(
      facebook::network::toAddress(IPAddress("aaaa:1::")));
  handler.getIpRoute(route, std::move(addr), RouterID(0));
  EXPECT_EQ(*route.counterID(), *counterID1);
}

TEST_F(ThriftTest, getRouteTableVerifyClassID) {
  ThriftHandler handler(sw_);
  auto bgpClient = static_cast<int16_t>(ClientID::BGPD);
  auto bgpClientAdmin = sw_->clientIdToAdminDistance(bgpClient);

  auto cli1_nhop6 = "2401:db00:2110:3001::0011";
  auto prefixA6 = "aaaa:1::0/64";
  auto addrA6 = folly::IPAddress("aaaa:1::0");
  std::optional<cfg::AclLookupClass> classID1(
      cfg::AclLookupClass::DST_CLASS_L3_DPR);

  // Add BGP routes with class ID
  handler.addUnicastRoute(
      bgpClient,
      makeUnicastRoute(
          prefixA6, cli1_nhop6, bgpClientAdmin, std::nullopt, classID1));

  std::vector<UnicastRoute> routeTable;
  bool found = false;
  handler.getRouteTable(routeTable);
  for (const auto& rt : routeTable) {
    if (rt.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*rt.classID(), *classID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  routeTable.resize(0);
  found = false;
  handler.getRouteTableByClient(routeTable, bgpClient);
  for (const auto& rt : routeTable) {
    if (rt.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*rt.classID(), *classID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  std::vector<RouteDetails> routeDetails;
  found = false;
  handler.getRouteTableDetails(routeDetails);
  for (const auto& route : routeDetails) {
    if (route.dest()->ip() == toBinaryAddress(addrA6)) {
      EXPECT_EQ(*route.classID(), *classID1);
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);

  UnicastRoute route;
  auto addr = std::make_unique<facebook::network::thrift::Address>(
      facebook::network::toAddress(IPAddress("aaaa:1::")));
  handler.getIpRoute(route, std::move(addr), RouterID(0));
  EXPECT_EQ(*route.classID(), *classID1);
}

TEST_F(ThriftTest, getLoopbackMode) {
  ThriftHandler handler(sw_);
  std::map<int32_t, PortLoopbackMode> port2LoopbackMode;
  handler.getAllPortLoopbackMode(port2LoopbackMode);
  EXPECT_EQ(port2LoopbackMode.size(), sw_->getState()->getPorts()->numNodes());
  std::for_each(
      port2LoopbackMode.begin(),
      port2LoopbackMode.end(),
      [](auto& portAndLbMode) {
        EXPECT_EQ(portAndLbMode.second, PortLoopbackMode::NONE);
      });
}

TEST_F(ThriftTest, setLoopbackMode) {
  ThriftHandler handler(sw_);
  std::map<int32_t, PortLoopbackMode> port2LoopbackMode;
  auto firstPort =
      sw_->getState()->getPorts()->cbegin()->second->cbegin()->second->getID();
  auto otherPortsUnchanged = [firstPort, this]() {
    for (auto& portMap : std::as_const(*sw_->getState()->getPorts())) {
      for (auto& port : std::as_const(*portMap.second)) {
        if (port.second->getID() != firstPort) {
          EXPECT_EQ(
              port.second->getLoopbackMode(), cfg::PortLoopbackMode::NONE);
        }
      }
    }
  };

  for (auto lbMode :
       {PortLoopbackMode::NIF,
        PortLoopbackMode::MAC,
        PortLoopbackMode::PHY,
        PortLoopbackMode::NONE}) {
    // MAC
    handler.setPortLoopbackMode(firstPort, lbMode);
    handler.getAllPortLoopbackMode(port2LoopbackMode);
    EXPECT_EQ(
        port2LoopbackMode.size(), sw_->getState()->getPorts()->numNodes());
    EXPECT_EQ(port2LoopbackMode.find(firstPort)->second, lbMode);
    otherPortsUnchanged();
  }
}

TEST_F(ThriftTest, programLedExternalState) {
  ThriftHandler handler(sw_);
  auto firstPort =
      sw_->getState()->getPorts()->cbegin()->second->cbegin()->second->getID();
  handler.setExternalLedState(
      firstPort, PortLedExternalState::EXTERNAL_FORCE_ON);
  auto port = sw_->getState()->getPorts()->getNode(firstPort);
  EXPECT_EQ(
      port->getLedPortExternalState(), PortLedExternalState::EXTERNAL_FORCE_ON);
}

TEST_F(ThriftTest, programInternalPhyPorts) {
  ThriftHandler handler(sw_);
  // Using Wedge100PlatformMapping in MockPlatform. Transceiver 4 is used by
  // eth1/5/[1-4], which is software port 1/2/3/4
  TransceiverID id = TransceiverID(4);
  constexpr auto kEnabledPort = 1;
  constexpr auto kCableLength = 3.5;
  constexpr auto kMediaInterface = MediaInterfaceCode::CWDM4_100G;
  constexpr auto kComplianceCode = ExtendedSpecComplianceCode::CWDM4_100G;
  constexpr auto kManagementInterface = TransceiverManagementInterface::CMIS;
  auto preparedTcvrInfo = [id,
                           kMediaInterface,
                           kComplianceCode,
                           kManagementInterface](double cableLength) {
    auto tcvrInfo = std::make_unique<TransceiverInfo>();
    tcvrInfo->tcvrState()->port() = id;
    tcvrInfo->tcvrState()->present() = true;
    Cable cable;
    cable.length() = cableLength;
    tcvrInfo->tcvrState()->cable() = cable;
    tcvrInfo->tcvrState()->transceiverManagementInterface() =
        kManagementInterface;
    TransceiverSettings tcvrSettings;
    std::vector<MediaInterfaceId> mediaInterfaces;
    for (int i = 0; i < 4; i++) {
      MediaInterfaceId intf;
      intf.lane() = i;
      intf.code() = kMediaInterface;
      mediaInterfaces.push_back(intf);
    }
    tcvrSettings.mediaInterface() = std::move(mediaInterfaces);
    tcvrInfo->tcvrState()->settings() = tcvrSettings;
    return tcvrInfo;
  };

  // Only enabled ports should be return
  auto checkProgrammedPorts =
      [&](const std::map<int32_t, cfg::PortProfileID>& programmedPorts) {
        EXPECT_EQ(programmedPorts.size(), 1);
        EXPECT_TRUE(
            programmedPorts.find(kEnabledPort) != programmedPorts.end());
        // Controlling port should be 100G
        EXPECT_EQ(
            programmedPorts.find(kEnabledPort)->second,
            cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER);

        // Make sure the enabled ports using the new profile config/pin configs
        auto tcvr = sw_->getState()->getTransceivers()->getNodeIf(id);
        std::optional<cfg::PlatformPortConfigOverrideFactor> factor;
        if (tcvr != nullptr) {
          factor = tcvr->toPlatformPortConfigOverrideFactor();
        }
        sw_->getPlatformMapping()->customizePlatformPortConfigOverrideFactor(
            factor);
        // Port must exist in the SwitchState
        const auto port =
            sw_->getState()->getPorts()->getNodeIf(PortID(kEnabledPort));
        EXPECT_TRUE(port->isEnabled());
        PlatformPortProfileConfigMatcher matcher{
            port->getProfileID(), port->getID(), factor};
        auto portProfileCfg =
            sw_->getPlatformMapping()->getPortProfileConfig(matcher);
        CHECK(portProfileCfg) << "No port profile config found with matcher:"
                              << matcher.toString();
        auto expectedProfileConfig = *portProfileCfg->iphy();
        const auto& expectedPinConfigs =
            sw_->getPlatformMapping()->getPortIphyPinConfigs(matcher);

        EXPECT_TRUE(expectedProfileConfig == port->getProfileConfig());
        EXPECT_TRUE(expectedPinConfigs == port->getPinConfigs());
      };

  std::map<int32_t, cfg::PortProfileID> programmedPorts;
  handler.programInternalPhyPorts(
      programmedPorts, preparedTcvrInfo(kCableLength), false);

  checkProgrammedPorts(programmedPorts);
  auto tcvr = sw_->getState()->getTransceivers()->getNode(id);
  EXPECT_EQ(tcvr->getID(), id);
  EXPECT_EQ(*tcvr->getCableLength(), kCableLength);
  EXPECT_EQ(*tcvr->getMediaInterface(), kMediaInterface);
  EXPECT_EQ(*tcvr->getManagementInterface(), kManagementInterface);

  // Now change the cable length
  const auto oldPort =
      sw_->getState()->getPorts()->getNodeIf(PortID(kEnabledPort));
  constexpr auto kCableLength2 = 1;
  std::map<int32_t, cfg::PortProfileID> programmedPorts2;
  handler.programInternalPhyPorts(
      programmedPorts2, preparedTcvrInfo(kCableLength2), false);

  checkProgrammedPorts(programmedPorts2);
  tcvr = sw_->getState()->getTransceivers()->getNode(id);
  EXPECT_EQ(tcvr->getID(), id);
  EXPECT_EQ(*tcvr->getCableLength(), kCableLength2);
  EXPECT_EQ(*tcvr->getMediaInterface(), kMediaInterface);
  EXPECT_EQ(*tcvr->getManagementInterface(), kManagementInterface);
  const auto newPort =
      sw_->getState()->getPorts()->getNodeIf(PortID(kEnabledPort));
  // Because we're using Wedge100PlatformMapping here, we should see pinConfig
  // change due to the cable length change
  EXPECT_TRUE(oldPort->getProfileConfig() == newPort->getProfileConfig());
  EXPECT_TRUE(oldPort->getPinConfigs() != newPort->getPinConfigs());

  // Using the same transceiver info to program and no new state created
  auto beforeGen = sw_->getState()->getGeneration();
  programmedPorts2.clear();
  handler.programInternalPhyPorts(
      programmedPorts2, preparedTcvrInfo(kCableLength2), false);
  checkProgrammedPorts(programmedPorts2);
  EXPECT_EQ(beforeGen, sw_->getState()->getGeneration());

  // Finally remove the transceiver
  auto unpresentTcvr = std::make_unique<TransceiverInfo>();
  unpresentTcvr->tcvrState()->port() = id;
  unpresentTcvr->tcvrState()->present() = false;
  std::map<int32_t, cfg::PortProfileID> programmedPorts3;
  handler.programInternalPhyPorts(
      programmedPorts3, std::move(unpresentTcvr), false);

  // Still return programmed ports even though no transceiver there
  checkProgrammedPorts(programmedPorts3);
  tcvr = sw_->getState()->getTransceivers()->getNodeIf(id);
  EXPECT_TRUE(tcvr == nullptr);

  // Remove the same Transceiver again, and make sure no new state created.
  beforeGen = sw_->getState()->getGeneration();
  programmedPorts3.clear();
  unpresentTcvr = std::make_unique<TransceiverInfo>();
  unpresentTcvr->tcvrState()->port() = id;
  unpresentTcvr->tcvrState()->present() = false;
  handler.programInternalPhyPorts(
      programmedPorts3, std::move(unpresentTcvr), false);
  // Still return programmed ports even though no transceiver there
  checkProgrammedPorts(programmedPorts3);
  tcvr = sw_->getState()->getTransceivers()->getNodeIf(id);
  EXPECT_TRUE(tcvr == nullptr);
  EXPECT_EQ(beforeGen, sw_->getState()->getGeneration());
}

TEST_F(ThriftTest, getConfigAppliedInfo) {
  ThriftHandler handler(sw_);
  // The SetUp() will applied an initialed config, so we should verify the
  // lastConfigAppliedInMs should be > 0 and < now.
  ConfigAppliedInfo initConfigAppliedInfo;
  handler.getConfigAppliedInfo(initConfigAppliedInfo);
  auto initConfigAppliedInMs = *initConfigAppliedInfo.lastAppliedInMs();
  EXPECT_GT(initConfigAppliedInMs, 0);
  // Thrift test should always trigger coldboot
  auto coldbootConfigAppliedTime =
      initConfigAppliedInfo.lastColdbootAppliedInMs();
  if (coldbootConfigAppliedTime) {
    EXPECT_EQ(*coldbootConfigAppliedTime, initConfigAppliedInMs);
  } else {
    throw FbossError("No coldboot config applied time");
  }

  // Adding sleep in case we immediatly check the last config applied time
  /* sleep override */
  usleep(1000);
  auto currentInMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
  EXPECT_LT(initConfigAppliedInMs, currentInMs);

  // Try to apply a new config, the lastConfigAppliedTime should changed
  // Adding sleep in case we apply a new config immediatly after the last config
  /* sleep override */
  usleep(1000);
  sw_->applyConfig(
      "New config with new speed profile", testConfigAWithLookupClasses());

  ConfigAppliedInfo newConfigAppliedInfo;
  handler.getConfigAppliedInfo(newConfigAppliedInfo);
  auto newConfigAppliedInMs = *newConfigAppliedInfo.lastAppliedInMs();
  EXPECT_GT(newConfigAppliedInMs, initConfigAppliedInMs);
  // Coldboot time should not change
  if (auto newColdbootConfigAppliedTime =
          newConfigAppliedInfo.lastColdbootAppliedInMs()) {
    EXPECT_EQ(*newColdbootConfigAppliedTime, *coldbootConfigAppliedTime);
  } else {
    throw FbossError("No coldboot config applied time");
  }
}

TEST_F(ThriftTest, applySpeedAndProfileMismatchConfig) {
  ThriftHandler handler(sw_);
  auto mismatchConfig = testConfigA();
  // Change the speed mismatch with the profile
  CHECK(!mismatchConfig.ports()->empty());
  mismatchConfig.ports()[0].state() = cfg::PortState::ENABLED;
  mismatchConfig.ports()[0].speed() = cfg::PortSpeed::TWENTYFIVEG;
  mismatchConfig.ports()[0].profileID() =
      cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER;

  EXPECT_THROW(
      sw_->applyConfig(
          "Mismatch config with wrong speed and profile", mismatchConfig),
      FbossError);
}

TEST_F(ThriftTest, getCurrentStateJSON) {
  ThriftHandler handler(sw_);
  std::string out;
  std::string in = "portMaps/id=0/1";
  handler.getCurrentStateJSON(out, std::make_unique<std::string>(in));
  auto dyn = folly::parseJson(out);
  EXPECT_EQ(dyn["portId"], 1);
  EXPECT_EQ(dyn["portName"], "port1");
  EXPECT_EQ(dyn["portState"], "ENABLED");

  in = "portMaps/id=0/1/portOperState";
  handler.getCurrentStateJSON(out, std::make_unique<std::string>(in));
  EXPECT_EQ(out, "false");

  // Empty thrift path
  in = "";
  EXPECT_THROW(
      handler.getCurrentStateJSON(out, std::make_unique<std::string>(in)),
      FbossError);

  // Invalid thrift path
  in = "invalid/path";
  EXPECT_THROW(
      handler.getCurrentStateJSON(out, std::make_unique<std::string>(in)),
      FbossError);
}

TEST_F(ThriftTest, getCurrentStateJSONForPaths) {
  ThriftHandler handler(sw_);
  std::map<std::string, std::string> pathToState;
  std::string in = "portMaps/id=0/1";
  std::vector<std::string> paths = {in};
  handler.getCurrentStateJSONForPaths(
      pathToState, std::make_unique<std::vector<std::string>>(paths));
  ASSERT_TRUE(pathToState.find(in) != pathToState.end());
  auto dyn = folly::parseJson(pathToState[in]);
  EXPECT_EQ(dyn["portId"], 1);
  EXPECT_EQ(dyn["portName"], "port1");
  EXPECT_EQ(dyn["portState"], "ENABLED");

  in = "portMaps/id=0/1/portOperState";
  paths = {in};
  handler.getCurrentStateJSONForPaths(
      pathToState, std::make_unique<std::vector<std::string>>(paths));
  ASSERT_TRUE(pathToState.find(in) != pathToState.end());
  EXPECT_EQ(pathToState[in], "false");

  // Empty thrift path
  paths = {""};
  EXPECT_THROW(
      handler.getCurrentStateJSONForPaths(
          pathToState, std::make_unique<std::vector<std::string>>(paths)),
      FbossError);

  // Invalid thrift path
  paths = {"invalid/path"};
  EXPECT_THROW(
      handler.getCurrentStateJSONForPaths(
          pathToState, std::make_unique<std::vector<std::string>>(paths)),
      FbossError);
}

template <bool enableIntfNbrTable>
struct EnableIntfNbrTable {
  static constexpr auto intfNbrTable = enableIntfNbrTable;
};

using NbrTableTypes =
    ::testing::Types<EnableIntfNbrTable<false>, EnableIntfNbrTable<true>>;

template <typename EnableIntfNbrTableT>
class ThriftTeFlowTest : public ::testing::Test {
  static auto constexpr intfNbrTable = EnableIntfNbrTableT::intfNbrTable;

 public:
  bool isIntfNbrTable() const {
    return intfNbrTable == true;
  }

  void SetUp() override {
    FLAGS_intf_nbr_tables = isIntfNbrTable();
    auto config = testConfigA();
    cfg::ExactMatchTableConfig tableConfig;
    tableConfig.name() = "TeFlowTable";
    tableConfig.dstPrefixLength() = 64;
    config.switchSettings()->exactMatchTableConfigs() = {tableConfig};
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();
    sw_->initialConfigApplied(std::chrono::steady_clock::now());

    if (isIntfNbrTable()) {
      sw_->getNeighborUpdater()->receivedNdpMineForIntf(
          kInterfaceA,
          folly::IPAddressV6(kNhopAddrA),
          kMacAddress,
          PortDescriptor(kPortIDA),
          ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
          0);
    } else {
      sw_->getNeighborUpdater()->receivedNdpMine(
          kVlanA,
          folly::IPAddressV6(kNhopAddrA),
          kMacAddress,
          PortDescriptor(kPortIDA),
          ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
          0);
    }

    if (isIntfNbrTable()) {
      sw_->getNeighborUpdater()->receivedNdpMineForIntf(
          kInterfaceB,
          folly::IPAddressV6(kNhopAddrB),
          kMacAddress,
          PortDescriptor(kPortIDB),
          ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
          0);
    } else {
      sw_->getNeighborUpdater()->receivedNdpMine(
          kVlanB,
          folly::IPAddressV6(kNhopAddrB),
          kMacAddress,
          PortDescriptor(kPortIDB),
          ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
          0);
    }

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
  }
  SwSwitch* sw_;
  std::unique_ptr<HwTestHandle> handle_;
};

TYPED_TEST_SUITE(ThriftTeFlowTest, NbrTableTypes);

TYPED_TEST(ThriftTeFlowTest, addRemoveTeFlow) {
  ThriftHandler handler(this->sw_);
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  auto flowEntry = makeFlow("100::1");
  teFlowEntries->emplace_back(flowEntry);
  handler.addTeFlows(std::move(teFlowEntries));
  auto state = this->sw_->getState();
  auto teFlowTable = state->getTeFlowTable();
  auto verifyEntry = [&teFlowTable](
                         const auto& flow,
                         const auto& nhop,
                         const auto& counter,
                         const auto& intf) {
    EXPECT_EQ(teFlowTable->numNodes(), 1);
    auto tableEntry = teFlowTable->getNodeIf(getTeFlowStr(flow));
    EXPECT_NE(tableEntry, nullptr);
    EXPECT_EQ(*tableEntry->getCounterID(), counter);
    EXPECT_EQ(tableEntry->getNextHops()->size(), 1);
    auto expectedNhop = toBinaryAddress(IPAddress(nhop));
    expectedNhop.ifName() = intf;
    auto nexthop = tableEntry->getNextHops()->cref(0)->toThrift();
    EXPECT_EQ(*nexthop.address(), expectedNhop);
    EXPECT_EQ(tableEntry->getResolvedNextHops()->size(), 1);
    auto resolveNexthop =
        tableEntry->getResolvedNextHops()->cref(0)->toThrift();
    EXPECT_EQ(*resolveNexthop.address(), expectedNhop);
  };
  verifyEntry(*flowEntry.flow(), kNhopAddrA, "counter0", "fboss1");

  // modify the entry
  auto flowEntry2 = makeFlow("100::1", kNhopAddrB, "counter1", "fboss55");
  auto newFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  newFlowEntries->emplace_back(flowEntry2);
  handler.addTeFlows(std::move(newFlowEntries));
  state = this->sw_->getState();
  teFlowTable = state->getTeFlowTable();
  verifyEntry(*flowEntry.flow(), kNhopAddrB, "counter1", "fboss55");

  auto teFlows = std::make_unique<std::vector<TeFlow>>();
  teFlows->emplace_back(*flowEntry.flow());
  handler.deleteTeFlows(std::move(teFlows));
  state = this->sw_->getState();
  teFlowTable = state->getTeFlowTable();
  auto tableEntry = teFlowTable->getNodeIf(getTeFlowStr(*flowEntry.flow()));
  EXPECT_EQ(tableEntry, nullptr);

  // add flows in bulk
  auto testPrefixes = {"100::1", "101::1", "102::1", "103::1"};
  auto bulkEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : testPrefixes) {
    bulkEntries->emplace_back(makeFlow(prefix));
  }
  handler.addTeFlows(std::move(bulkEntries));
  state = this->sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 4);

  // bulk delete
  auto flowsToDelete = {"100::1", "101::1"};
  auto deletionFlows = std::make_unique<std::vector<TeFlow>>();
  for (const auto& prefix : flowsToDelete) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    deletionFlows->emplace_back(flow);
  }
  handler.deleteTeFlows(std::move(deletionFlows));
  state = this->sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 2);
  for (const auto& prefix : flowsToDelete) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    EXPECT_EQ(teFlowTable->getNodeIf(getTeFlowStr(flow)), nullptr);
  }
}

TYPED_TEST(ThriftTeFlowTest, syncTeFlows) {
  auto initalPrefixes = {"100::1", "101::1", "102::1", "103::1"};
  ThriftHandler handler(this->sw_);
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : initalPrefixes) {
    auto flowEntry = makeFlow(prefix);
    teFlowEntries->emplace_back(flowEntry);
  }
  handler.addTeFlows(std::move(teFlowEntries));
  auto state = this->sw_->getState();
  auto teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 4);

  // Ensure that all entries are created
  for (const auto& prefix : initalPrefixes) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    auto tableEntry = teFlowTable->getNodeIf(getTeFlowStr(flow));
    EXPECT_NE(tableEntry, nullptr);
  }

  TeFlow flow;
  flow.dstPrefix() = ipPrefix("100::1", 64);
  flow.srcPort() = 100;
  auto teflowEntryBeforeSync = teFlowTable->getNodeIf(getTeFlowStr(flow));
  auto syncPrefixes = {"100::1", "101::1", "104::1"};
  auto syncFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : syncPrefixes) {
    auto flowEntry = makeFlow(prefix);
    syncFlowEntries->emplace_back(flowEntry);
  }
  handler.syncTeFlows(std::move(syncFlowEntries));
  state = this->sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 3);
  // Ensure that newly added entries are present
  for (const auto& prefix : syncPrefixes) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    auto tableEntry = teFlowTable->getNodeIf(getTeFlowStr(flow));
    EXPECT_NE(tableEntry, nullptr);
  }
  // Ensure that missing entries are removed
  for (const auto& prefix : {"102::1", "103::1"}) {
    TeFlow flow;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    flow.srcPort() = 100;
    auto tableEntry = teFlowTable->getNodeIf(getTeFlowStr(flow));
    EXPECT_EQ(tableEntry, nullptr);
  }
  // Ensure that pointer to entries and contents are same
  auto teflowEntryAfterSync = teFlowTable->getNodeIf(getTeFlowStr(flow));
  EXPECT_EQ(teflowEntryBeforeSync, teflowEntryAfterSync);
  EXPECT_EQ(*teflowEntryBeforeSync, *teflowEntryAfterSync);
  // Sync with no change in entries and verify table is same
  auto syncFlowEntries2 = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : syncPrefixes) {
    auto flowEntry = makeFlow(prefix);
    syncFlowEntries2->emplace_back(flowEntry);
  }
  handler.syncTeFlows(std::move(syncFlowEntries2));
  state = this->sw_->getState();
  auto teFlowTableAfterSync = state->getTeFlowTable();
  // Ensure teflow table pointers and contents are same
  EXPECT_EQ(teFlowTable, teFlowTableAfterSync);
  EXPECT_EQ(*teFlowTable, *teFlowTableAfterSync);
  // Update an entry and check the pointer and content changed
  flow.dstPrefix() = ipPrefix("104::1", 64);
  flow.srcPort() = 100;
  teflowEntryBeforeSync = teFlowTable->getNodeIf(getTeFlowStr(flow));
  auto updateEntries = std::make_unique<std::vector<FlowEntry>>();
  auto flowEntry1 = makeFlow("100::1");
  auto flowEntry2 = makeFlow("104::1", kNhopAddrA, "counter1", "fboss1");
  updateEntries->emplace_back(flowEntry1);
  updateEntries->emplace_back(flowEntry2);
  handler.syncTeFlows(std::move(updateEntries));
  state = this->sw_->getState();
  teFlowTable = state->getTeFlowTable();
  teflowEntryAfterSync = teFlowTable->getNodeIf(getTeFlowStr(flow));
  // Ensure that pointer to entries and contents are different
  EXPECT_NE(teflowEntryBeforeSync, teflowEntryAfterSync);
  EXPECT_NE(*teflowEntryBeforeSync, *teflowEntryAfterSync);
  // sync flows with no entries
  auto nullFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  handler.syncTeFlows(std::move(nullFlowEntries));
  state = this->sw_->getState();
  teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 0);
}

TYPED_TEST(ThriftTeFlowTest, getTeFlowDetails) {
  auto testPrefixes = {"100::1", "101::1", "102::1", "103::1"};
  ThriftHandler handler(this->sw_);
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  for (const auto& prefix : testPrefixes) {
    auto flowEntry = makeFlow(prefix);
    teFlowEntries->emplace_back(flowEntry);
  }
  handler.addTeFlows(std::move(teFlowEntries));
  auto state = this->sw_->getState();
  auto teFlowTable = state->getTeFlowTable();
  EXPECT_EQ(teFlowTable->numNodes(), 4);

  std::vector<TeFlowDetails> flowDetails;
  handler.getTeFlowTableDetails(flowDetails);
  EXPECT_EQ(flowDetails.size(), teFlowTable->numNodes());

  auto idx = 0;
  for (const auto& prefix : testPrefixes) {
    TeFlow flow;
    flow.srcPort() = 100;
    flow.dstPrefix() = ipPrefix(prefix, 64);
    auto flowDetail = flowDetails[idx++];
    auto tableEntry = state->getTeFlowTable()->getNodeIf(getTeFlowStr(flow));
    EXPECT_EQ(flowDetail.enabled(), tableEntry->getEnabled());
    EXPECT_EQ(flowDetail.counterID(), tableEntry->getCounterID()->toThrift());
    EXPECT_EQ(flowDetail.nexthops(), tableEntry->getNextHops()->toThrift());
    EXPECT_EQ(
        flowDetail.resolvedNexthops(),
        tableEntry->getResolvedNextHops()->toThrift());
  }
}

TYPED_TEST(ThriftTeFlowTest, teFlowUpdateHwProtection) {
  ThriftHandler handler(this->sw_);
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  auto flowEntry = makeFlow("100::1");
  teFlowEntries->emplace_back(flowEntry);

  // wait for any neighbour updates
  this->sw_->getNeighborUpdater()->waitForPendingUpdates();
  waitForBackgroundThread(this->sw_);
  waitForStateUpdates(this->sw_);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(this->sw_, stateChangedImpl(_))
      .WillOnce(Return(this->sw_->getState()));

  EXPECT_THROW(
      {
        try {
          handler.addTeFlows(std::move(teFlowEntries));

        } catch (const FbossTeUpdateError& flowError) {
          EXPECT_EQ(flowError.failedAddUpdateFlows()->size(), 1);
          EXPECT_EQ(
              flowError.failedAddUpdateFlows()[0], makeFlow("100::1").flow());
          throw;
        }
      },
      FbossTeUpdateError);
}

TYPED_TEST(ThriftTeFlowTest, teFlowSyncUpdateHwProtection) {
  ThriftHandler handler(this->sw_);
  auto teFlowEntries = std::make_unique<std::vector<FlowEntry>>();
  auto flowEntry = makeFlow("100::1");
  teFlowEntries->emplace_back(flowEntry);
  // Fail HW update by returning current state
  EXPECT_HW_CALL(this->sw_, stateChangedImpl(_))
      .WillOnce(Return(this->sw_->getState()));

  EXPECT_THROW(
      {
        try {
          handler.syncTeFlows(std::move(teFlowEntries));

        } catch (const FbossTeUpdateError& flowError) {
          EXPECT_EQ(flowError.failedAddUpdateFlows()->size(), 1);
          EXPECT_EQ(
              flowError.failedAddUpdateFlows()[0], makeFlow("100::1").flow());
          throw;
        }
      },
      FbossTeUpdateError);
}
