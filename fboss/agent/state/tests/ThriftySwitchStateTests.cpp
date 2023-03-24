/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

// #include <folly/IPAddress.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/AclTableGroupMap.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/test/TestUtils.h"
#include "folly/IPAddress.h"
#include "folly/IPAddressV4.h"

using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {

void verifySwitchStateSerialization(const SwitchState& state) {
  auto stateBack = SwitchState::fromThrift(state.toThrift());
  EXPECT_EQ(state, *stateBack);
}
} // namespace

TEST(ThriftySwitchState, BasicTest) {
  auto state = SwitchState();
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, PortMap) {
  state::PortFields portFields1;
  portFields1.portId() = PortID(1);
  portFields1.portName() = "eth2/1/1";
  auto port1 = std::make_shared<Port>(std::move(portFields1));
  state::PortFields portFields2;
  portFields2.portId() = PortID(2);
  portFields2.portName() = "eth2/2/1";
  auto port2 = std::make_shared<Port>(std::move(portFields2));

  auto portMap = std::make_shared<PortMap>();
  portMap->addPort(port1);
  portMap->addPort(port2);
  validateThriftMapMapSerialization(*portMap);

  auto state = SwitchState();
  state.resetPorts(portMap);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, VlanMap) {
  auto vlan1 = std::make_shared<Vlan>(VlanID(1), std::string("vlan1"));
  auto vlan2 = std::make_shared<Vlan>(VlanID(2), std::string("vlan2"));

  vlan1->setDhcpV4Relay(IPAddressV4("1.2.3.4"));
  vlan1->setDhcpV4RelayOverrides(
      {{MacAddress("02:00:00:00:00:02"), IPAddressV4("1.2.3.4")}});
  vlan1->setInterfaceID(InterfaceID(1));

  auto arpTable = std::make_shared<ArpTable>();
  arpTable->addEntry(
      IPAddressV4("1.2.3.4"),
      MacAddress("02:00:00:00:00:03"),
      PortDescriptor(PortID(1)),
      InterfaceID(1));
  vlan1->setArpTable(arpTable);

  auto ndpTable = std::make_shared<NdpTable>();
  ndpTable->addEntry(
      IPAddressV6("2401:db00:21:70cb:face:0:96:0"),
      MacAddress("02:00:00:00:00:04"),
      PortDescriptor(PortID(2)),
      InterfaceID(2));
  vlan1->setNdpTable(ndpTable);

  auto arpResponseTable = std::make_shared<ArpResponseTable>();
  arpResponseTable->setEntry(
      IPAddressV4("1.2.3.5"), MacAddress("02:00:00:00:00:06"), InterfaceID(3));
  vlan1->setArpResponseTable(arpResponseTable);

  auto ndpResponseTable = std::make_shared<NdpResponseTable>();
  ndpResponseTable->setEntry(
      IPAddressV6("2401:db00:21:70cb:face:0:96:1"),
      MacAddress("02:00:00:00:00:07"),
      InterfaceID(4));
  vlan1->setNdpResponseTable(ndpResponseTable);

  auto macTable = std::make_shared<MacTable>();
  auto macEntry = std::make_shared<MacEntry>(
      MacAddress("02:00:00:00:00:08"),
      PortDescriptor(PortID(4)),
      std::optional<cfg::AclLookupClass>(cfg::AclLookupClass::CLASS_DROP));
  macTable->addEntry(macEntry);

  vlan2->setDhcpV6Relay(IPAddressV6("2401:db00:21:70cb:face:0:96:0"));
  vlan2->setDhcpV6RelayOverrides(
      {{MacAddress("02:00:00:00:00:03"),
        IPAddressV6("2401:db00:21:70cb:face:0:96:0")}});

  auto vlanMap = std::make_shared<VlanMap>();
  vlanMap->addVlan(vlan1);
  vlanMap->addVlan(vlan2);

  auto state = SwitchState();
  state.resetVlans(vlanMap);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, AclMap) {
  auto acl1 = std::make_shared<AclEntry>(1, std::string("acl1"));
  auto acl2 = std::make_shared<AclEntry>(2, std::string("acl2"));

  auto aclMap = std::make_shared<AclMap>();
  aclMap->addEntry(acl1);
  aclMap->addEntry(acl2);

  auto state = SwitchState();
  state.resetAcls(aclMap);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, TransceiverMap) {
  auto transceiver1 = std::make_shared<TransceiverSpec>(TransceiverID(1));
  auto transceiver2 = std::make_shared<TransceiverSpec>(TransceiverID(2));

  auto transceiverMap = std::make_shared<TransceiverMap>();
  transceiverMap->addTransceiver(transceiver1);
  transceiverMap->addTransceiver(transceiver2);

  auto state = SwitchState();
  state.resetTransceivers(transceiverMap);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, BufferPoolCfgMap) {
  const std::string buffer1 = "pool1";
  const std::string buffer2 = "pool2";
  auto pool1 = std::make_shared<BufferPoolCfg>(buffer1);
  auto pool2 = std::make_shared<BufferPoolCfg>(buffer2);
  pool1->setHeadroomBytes(100);
  pool2->setHeadroomBytes(200);

  auto map = std::make_shared<BufferPoolCfgMap>();
  map->addNode(pool1);
  map->addNode(pool2);

  auto state = SwitchState();
  state.resetBufferPoolCfgs(map);
}

TEST(ThriftySwitchState, QosPolicyMap) {
  const std::string kQosPolicy1Name = "qosPolicy1";
  const std::string kQosPolicy2Name = "qosPolicy2";
  auto qosPolicy1 = std::make_shared<QosPolicy>(kQosPolicy1Name, DscpMap());
  auto qosPolicy2 = std::make_shared<QosPolicy>(kQosPolicy2Name, DscpMap());

  auto map = std::make_shared<QosPolicyMap>();
  map->addNode(qosPolicy1);
  map->addNode(qosPolicy2);

  auto state = SwitchState();
  state.resetQosPolicies(map);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, SflowCollectorMap) {
  auto sflowCollector1 = std::make_shared<SflowCollector>(
      std::string("1.2.3.4"), static_cast<uint16_t>(8080));
  auto sflowCollector2 = std::make_shared<SflowCollector>(
      std::string("2::3"), static_cast<uint16_t>(9090));

  auto map = std::make_shared<SflowCollectorMap>();
  map->addNode(sflowCollector1);
  map->addNode(sflowCollector2);

  auto state = SwitchState();
  state.resetSflowCollectors(map);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, AggregatePortMap) {
  auto platform = createMockPlatform();
  auto startState = testStateA();
  std::vector<int> memberPort1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  std::vector<int> memberPort2 = {11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

  auto config = testConfigA();
  config.aggregatePorts()->resize(2);
  *config.aggregatePorts()[0].key() = 55;
  *config.aggregatePorts()[0].name() = "lag55";
  *config.aggregatePorts()[0].description() = "upwards facing link-bundle";
  (*config.aggregatePorts()[0].memberPorts()).resize(10);
  for (int i = 0; i < memberPort1.size(); ++i) {
    *config.aggregatePorts()[0].memberPorts()[i].memberPortID() =
        memberPort1[i];
  }
  *config.aggregatePorts()[1].key() = 155;
  *config.aggregatePorts()[1].name() = "lag155";
  *config.aggregatePorts()[1].description() = "downwards facing link-bundle";
  (*config.aggregatePorts()[1].memberPorts()).resize(10);
  *config.aggregatePorts()[1].memberPorts()[0].memberPortID() = 1;
  for (int i = 0; i < memberPort2.size(); ++i) {
    *config.aggregatePorts()[1].memberPorts()[i].memberPortID() =
        memberPort2[i];
  }

  auto endState = publishAndApplyConfig(startState, &config, platform.get());
  ASSERT_NE(nullptr, endState);
  auto aggPorts = endState->getAggregatePorts();

  auto state = SwitchState();
  state.resetAggregatePorts(aggPorts);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, AclTableGroupMap) {
  MockPlatform platform;

  const std::string kTable1 = "table1";
  const std::string kTable2 = "table2";
  const std::string kAcl1a = "acl1a";
  const std::string kAcl1b = "acl1b";
  const std::string kAcl2a = "acl2a";
  const std::string kGroup1 = "group1";
  int priority1 = AclTable::kDataplaneAclMaxPriority;
  int priority2 = AclTable::kDataplaneAclMaxPriority;
  const cfg::AclStage kAclStage1 = cfg::AclStage::INGRESS;

  auto entry1a = std::make_shared<AclEntry>(priority1++, kAcl1a);
  entry1a->setActionType(cfg::AclActionType::DENY);
  auto entry1b = std::make_shared<AclEntry>(priority1++, kAcl1b);
  entry1b->setActionType(cfg::AclActionType::DENY);
  auto map1 = std::make_shared<AclMap>();
  map1->addEntry(entry1a);
  map1->addEntry(entry1b);
  auto table1 = std::make_shared<AclTable>(1, kTable1);
  table1->setAclMap(map1);

  auto entry2a = std::make_shared<AclEntry>(priority2++, kAcl2a);
  entry2a->setActionType(cfg::AclActionType::DENY);
  auto map2 = std::make_shared<AclMap>();
  map2->addEntry(entry2a);
  auto table2 = std::make_shared<AclTable>(2, kTable2);
  table2->setAclMap(map2);

  auto tableMap = std::make_shared<AclTableMap>();
  tableMap->addTable(table1);
  tableMap->addTable(table2);
  auto tableGroup = std::make_shared<AclTableGroup>(kAclStage1);
  tableGroup->setAclTableMap(tableMap);
  tableGroup->setName(kGroup1);

  auto tableGroups = std::make_shared<AclTableGroupMap>();
  tableGroups->addAclTableGroup(tableGroup);

  auto state = SwitchState();
  state.resetAclTableGroups(tableGroups);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, InterfaceMap) {
  auto platform = createMockPlatform();
  auto startState = testStateA();

  auto config = testConfigA();
  config.vlans()->resize(2);
  *config.vlans()[0].id() = 1;
  config.vlans()[0].intfID() = 1;

  *config.vlans()[1].id() = 55;
  config.vlans()[1].intfID() = 55;

  config.interfaces()->resize(2);
  *config.interfaces()[0].intfID() = 1;
  *config.interfaces()[0].vlanID() = 1;
  config.interfaces()[0].mac() = "00:00:00:00:00:11";

  *config.interfaces()[1].intfID() = 55;
  *config.interfaces()[1].vlanID() = 55;
  config.interfaces()[1].mac() = "00:00:00:00:00:55";

  auto endState = publishAndApplyConfig(startState, &config, platform.get());
  ASSERT_NE(nullptr, endState);
  auto interfaces = endState->getInterfaces();

  auto state = SwitchState();
  state.resetIntfs(interfaces);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, IpAddressConversion) {
  for (auto ipStr : {"212.12.45.89", "2401:ab:de::30"}) {
    auto ip_0 = folly::IPAddress(ipStr);
    auto addr_0 = facebook::network::toBinaryAddress(ip_0);

    auto ip_0_dynamic = ThriftyUtils::toFollyDynamic(ip_0);
    auto addr_0_dynamic = ThriftyUtils::toFollyDynamic(addr_0);

    auto addr_1 = ThriftyUtils::toThriftBinaryAddress(ip_0_dynamic);
    auto ip_1 = ThriftyUtils::toFollyIPAddress(addr_0_dynamic);

    auto ip_1_dynamic = ThriftyUtils::toFollyDynamic(ip_0);
    auto addr_1_dynamic = ThriftyUtils::toFollyDynamic(addr_0);

    EXPECT_EQ(ip_0, ip_1);
    EXPECT_EQ(addr_0, addr_1);

    EXPECT_EQ(ip_0_dynamic, ip_1_dynamic);
    EXPECT_EQ(addr_0_dynamic, addr_1_dynamic);
  }
}

TEST(ThriftySwitchState, MultiMaps) {
  state::SwitchState stateThrift0{};

  stateThrift0.fibs()->emplace(
      0,
      makeFibContainerFields(
          0,
          {"1.1.1.1/24", "2.1.1.1/24", "3.1.1.1/24"},
          {"1::1/64", "2::1/64", "3::1/64"}));

  stateThrift0.fibs()->emplace(
      1,
      makeFibContainerFields(
          1,
          {"10.1.1.1/24", "20.1.1.1/24", "30.1.1.1/24"},
          {"10::1/64", "20::1/64", "30::1/64"}));

  auto state = SwitchState::fromThrift(stateThrift0);
  auto stateThrift1 = state->toThrift();

  // backward compatibility
  EXPECT_EQ(*stateThrift0.fibs(), *stateThrift1.fibs());
  // forward compatibility
  EXPECT_EQ(*stateThrift0.fibs(), stateThrift1.fibsMap()->at("id=0"));
}

TEST(ThriftySwitchState, EmptyMultiMap) {
  auto state = SwitchState::fromThrift(state::SwitchState{});
  EXPECT_THROW(state->getMirrors(), std::exception);
  EXPECT_THROW(state->getFibs(), std::exception);
}
