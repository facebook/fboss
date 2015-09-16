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
#include "fboss/agent/gen-cpp/switch_config_types.h"
#include "fboss/agent/state/ArpResponseTable.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/NdpResponseTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/VlanMap.h"

#include "fboss/agent/state/NodeBase-defs.h"

// Include NeighborResponseTable-defs.h to instantiate code
// in that file so that it does not get dropped from static
// (switch)state library.
#include "fboss/agent/state/NeighborResponseTable-defs.h"

using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using folly::to;
using std::make_pair;
using std::string;

namespace {
constexpr auto kVlanId = "vlanId";
constexpr auto kVlanMtu = "vlanMTU";
constexpr auto kVlanName = "vlanName";
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
}

namespace facebook { namespace fboss {

folly::dynamic VlanFields::PortInfo::toFollyDynamic() const {
  folly::dynamic port = folly::dynamic::object;
  port[kTagged] = tagged;
  return port;
}

VlanFields::PortInfo
VlanFields::PortInfo::fromFollyDynamic(const folly::dynamic& json) {
  return PortInfo(json[kTagged].asBool());
}

VlanFields::VlanFields(VlanID _id, string _name)
  : id(_id),
    name(std::move(_name)),
    arpTable(new ArpTable),
    arpResponseTable(new ArpResponseTable),
    ndpTable(new NdpTable),
    ndpResponseTable(new NdpResponseTable) {
}

VlanFields::VlanFields(VlanID _id,
                       string _name,
                       IPAddressV4 v4Relay,
                       IPAddressV6 v6Relay,
                       MemberPorts&& ports)
  : id(_id),
    name(std::move(_name)),
    dhcpV4Relay(v4Relay),
    dhcpV6Relay(v6Relay),
    ports(std::move(ports)),
    arpTable(new ArpTable),
    arpResponseTable(new ArpResponseTable),
    ndpTable(new NdpTable),
    ndpResponseTable(new NdpResponseTable) {
}

folly::dynamic VlanFields::toFollyDynamic() const {
  folly::dynamic vlan = folly::dynamic::object;
  vlan[kVlanId] = static_cast<uint16_t>(id);
  vlan[kVlanName] = name;
  vlan[kDhcpV4Relay] = dhcpV4Relay.str();
  vlan[kDhcpV6Relay] = dhcpV6Relay.str();
  vlan[kDhcpV4RelayOverrides] = folly::dynamic::object;
  for (const auto& o: dhcpRelayOverridesV4) {
    vlan[kDhcpV4RelayOverrides][o.first.toString()] = o.second.str();
  }
  vlan[kDhcpV6RelayOverrides] = folly::dynamic::object;
  for (const auto& o: dhcpRelayOverridesV6) {
    vlan[kDhcpV6RelayOverrides][o.first.toString()] = o.second.str();
  }
  folly::dynamic memberPorts = folly::dynamic::object;
  for (const auto& port: ports) {
    folly::dynamic portInfo = folly::dynamic::object;
    memberPorts[to<string>(static_cast<uint16_t>(port.first))] =
        port.second.toFollyDynamic();
  }
  vlan[kMemberPorts] = memberPorts;
  vlan[kArpTable] = arpTable->toFollyDynamic();
  vlan[kNdpTable] = ndpTable->toFollyDynamic();
  vlan[kArpResponseTable] = arpResponseTable->toFollyDynamic();
  vlan[kNdpResponseTable] = ndpResponseTable->toFollyDynamic();
  return vlan;
}

VlanFields VlanFields::fromFollyDynamic(const folly::dynamic& vlanJson) {
  VlanFields vlan(VlanID(vlanJson[kVlanId].asInt()),
      vlanJson[kVlanName].asString().toStdString());
  vlan.dhcpV4Relay = folly::IPAddressV4(
      vlanJson[kDhcpV4Relay].stringPiece());
  vlan.dhcpV6Relay = folly::IPAddressV6(
      vlanJson[kDhcpV6Relay].stringPiece());
  for (const auto& o: vlanJson[kDhcpV4RelayOverrides].items()) {
    vlan.dhcpRelayOverridesV4[MacAddress(o.first.asString().toStdString())] =
        folly::IPAddressV4(o.second.stringPiece());
  }
  for (const auto& o: vlanJson[kDhcpV6RelayOverrides].items()) {
    vlan.dhcpRelayOverridesV6[MacAddress(o.first.asString().toStdString())] =
        folly::IPAddressV6(o.second.stringPiece());
  }
  for (const auto& portInfo: vlanJson[kMemberPorts].items()) {
    vlan.ports.emplace(PortID(to<uint16_t>(portInfo.first.asString())),
          PortInfo::fromFollyDynamic(portInfo.second));
  }
  vlan.arpTable = ArpTable::fromFollyDynamic(vlanJson[kArpTable]);
  vlan.ndpTable = NdpTable::fromFollyDynamic(vlanJson[kNdpTable]);
  vlan.arpResponseTable = ArpResponseTable::fromFollyDynamic(
      vlanJson[kArpResponseTable]);
  vlan.ndpResponseTable = NdpResponseTable::fromFollyDynamic(
      vlanJson[kNdpResponseTable]);
  return vlan;
}

Vlan::Vlan(VlanID id, string name)
  : NodeBaseT(id, std::move(name)) {
}

Vlan::Vlan(const cfg::Vlan* config, MemberPorts ports)
  : NodeBaseT(VlanID(config->id),
              config->name,
              (config->__isset.dhcpRelayAddressV4 ?
               IPAddressV4(config->dhcpRelayAddressV4) : IPAddressV4()),
              (config->__isset.dhcpRelayAddressV6 ?
               IPAddressV6(config->dhcpRelayAddressV6) : IPAddressV6()),
               std::move(ports)) {

  DhcpV4OverrideMap map4;
  for (const auto& o: config->dhcpRelayOverridesV4) {
    map4[MacAddress(o.first)] = folly::IPAddressV4(o.second);
  }
  setDhcpV4RelayOverrides(map4);

  DhcpV6OverrideMap map6;
  for (const auto& o: config->dhcpRelayOverridesV6) {
    map6[MacAddress(o.first)] = folly::IPAddressV6(o.second);
  }
  setDhcpV6RelayOverrides(map6);
}

Vlan* Vlan::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  VlanMap* vlans = (*state)->getVlans()->modify(state);
  auto newVlan = clone();
  auto* ptr = newVlan.get();
  vlans->updateVlan(std::move(newVlan));
  return ptr;
}

void Vlan::addPort(PortID id, bool tagged) {
  writableFields()->ports.insert(make_pair(id, PortID(tagged)));
}

template class NodeBaseT<Vlan, VlanFields>;

}} // facebook::fboss
