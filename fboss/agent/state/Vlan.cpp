/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/Vlan.h"

#include <folly/String.h>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/MacTable.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/VlanMap.h"

#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/types.h"

using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::to;
using std::make_pair;
using std::make_shared;
using std::string;

namespace {
constexpr auto kVlanId = "vlanId";
constexpr auto kVlanMtu = "vlanMTU";
constexpr auto kVlanName = "vlanName";
constexpr auto kIntfID = "intfID";
constexpr auto kDhcpV4Relay = "dhcpV4Relay";
constexpr auto kDhcpV4RelayOverrides = "dhcpRelayOverridesV4";
constexpr auto kDhcpV6Relay = "dhcpV6Relay";
constexpr auto kDhcpV6RelayOverrides = "dhcpRelayOverridesV6";
constexpr auto kMemberPorts = "memberPorts";
constexpr auto kTagged = "tagged";
constexpr auto kArpTable = "arpTable";
constexpr auto kArpResponseTable = "arpResponseTable";
constexpr auto kNdpTable = "ndpTable";
constexpr auto kNdpResponseTable = "ndpResponseTable";
constexpr auto kMacTable = "macTable";
} // namespace

namespace facebook::fboss {

Vlan::Vlan(VlanID id, string name) {
  set<switch_state_tags::vlanId>(id);
  setName(name);
}

Vlan::Vlan(const cfg::Vlan* config, MemberPorts ports) {
  set<switch_state_tags::vlanId>(*config->id());
  setName(*config->name());
  if (auto intfId = config->intfID()) {
    setInterfaceID(InterfaceID(*intfId));
  }
  if (auto dhcpRelayAddressV4 = config->dhcpRelayAddressV4()) {
    setDhcpV4Relay(folly::IPAddressV4(*dhcpRelayAddressV4));
  }
  if (auto dhcpRelayAddressV6 = config->dhcpRelayAddressV6()) {
    setDhcpV6Relay(folly::IPAddressV6(*dhcpRelayAddressV6));
  }
  setPorts(std::move(ports));
  DhcpV4OverrideMap map4;
  for (const auto& o : config->dhcpRelayOverridesV4().value_or({})) {
    map4[MacAddress(o.first)] = folly::IPAddressV4(o.second);
  }
  setDhcpV4RelayOverrides(map4);

  DhcpV6OverrideMap map6;
  for (const auto& o : config->dhcpRelayOverridesV6().value_or({})) {
    map6[MacAddress(o.first)] = folly::IPAddressV6(o.second);
  }
  setDhcpV6RelayOverrides(map6);
}

Vlan* Vlan::modify(
    std::shared_ptr<SwitchState>* state,
    const HwSwitchMatcher& matcher) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  MultiSwitchVlanMap* vlans = (*state)->getVlans()->modify(state);
  auto newVlan = clone();
  auto* ptr = newVlan.get();
  vlans->updateNode(std::move(newVlan), matcher);
  return ptr;
}

void Vlan::addPort(PortID id, bool tagged) {
  ref<switch_state_tags::ports>()->emplace(id, tagged);
}

template class ThriftStructNode<Vlan, state::VlanFields>;

} // namespace facebook::fboss
