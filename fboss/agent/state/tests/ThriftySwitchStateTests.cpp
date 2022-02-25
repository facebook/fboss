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
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

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
