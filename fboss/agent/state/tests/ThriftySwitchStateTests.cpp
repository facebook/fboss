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
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/test/TestUtils.h"
#include "folly/IPAddress.h"

using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::IPAddressV6;

namespace {

void verifySwitchStateSerialization(const SwitchState& state) {
  auto stateBack = SwitchState::fromThrift(state.toThrift());
  EXPECT_EQ(state, *stateBack);
  stateBack = SwitchState::fromFollyDynamic(state.toFollyDynamic());
  EXPECT_EQ(state, *stateBack);
  // verify dynamic is still json serializable
  EXPECT_NO_THROW(folly::toJson(state.toFollyDynamic()));
}
} // namespace

TEST(ThriftySwitchState, BasicTest) {
  auto state = SwitchState();
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, PortMap) {
  auto port1 = std::make_shared<Port>(PortID(1), "eth2/1/1");
  auto port2 = std::make_shared<Port>(PortID(2), "eth2/2/1");
  auto portMap = std::make_shared<PortMap>();
  portMap->addPort(port1);
  portMap->addPort(port2);
  validateThriftyMigration(*portMap);

  auto state = SwitchState();
  state.resetPorts(portMap);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, VlanMap) {
  auto vlan1 = std::make_shared<Vlan>(VlanID(1), "vlan1");
  auto vlan2 = std::make_shared<Vlan>(VlanID(2), "vlan2");

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
      cfg::AclLookupClass::CLASS_DROP);
  macTable->addEntry(macEntry);

  vlan2->setDhcpV6Relay(IPAddressV6("2401:db00:21:70cb:face:0:96:0"));
  vlan2->setDhcpV6RelayOverrides(
      {{MacAddress("02:00:00:00:00:03"),
        IPAddressV6("2401:db00:21:70cb:face:0:96:0")}});

  auto vlanMap = std::make_shared<VlanMap>();
  vlanMap->addVlan(vlan1);
  vlanMap->addVlan(vlan2);

  validateThriftyMigration(*vlanMap);

  auto state = SwitchState();
  state.resetVlans(vlanMap);
  verifySwitchStateSerialization(state);
}

TEST(ThriftySwitchState, AclMap) {
  auto acl1 = std::make_shared<AclEntry>(1, "acl1");
  auto acl2 = std::make_shared<AclEntry>(2, "acl2");

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

  validateThriftyMigration(*transceiverMap);

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
