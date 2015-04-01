/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"

#include "fboss/agent/state/NodeBase-defs.h"

using std::make_shared;
using std::shared_ptr;

namespace {
constexpr auto kInterfaces = "interfaces";
constexpr auto kPorts = "ports";
constexpr auto kVlans = "vlans";
constexpr auto kRouteTables = "routeTables";
constexpr auto kDefaultVlan = "defaultVlan";
}

namespace facebook { namespace fboss {

SwitchStateFields::SwitchStateFields()
  : ports(make_shared<PortMap>()),
    vlans(make_shared<VlanMap>()),
    interfaces(make_shared<InterfaceMap>()),
    routeTables(make_shared<RouteTableMap>()) {
}

folly::dynamic SwitchStateFields::toFollyDynamic() const {
  folly::dynamic switchState = folly::dynamic::object;
  switchState[kInterfaces] = interfaces->toFollyDynamic();
  switchState[kPorts] = ports->toFollyDynamic();
  switchState[kVlans] = vlans->toFollyDynamic();
  switchState[kRouteTables] = routeTables->toFollyDynamic();
  switchState[kDefaultVlan] = static_cast<uint32_t>(defaultVlan);
  return switchState;
}

SwitchStateFields
SwitchStateFields::fromFollyDynamic(const folly::dynamic& swJson) {
  SwitchStateFields switchState;
  switchState.interfaces = InterfaceMap::fromFollyDynamic(
        swJson[kInterfaces]);
  switchState.ports = PortMap::fromFollyDynamic(swJson[kPorts]);
  switchState.vlans = VlanMap::fromFollyDynamic(swJson[kVlans]);
  switchState.routeTables = RouteTableMap::fromFollyDynamic(
      swJson[kRouteTables]);
  switchState.defaultVlan = VlanID(swJson[kDefaultVlan].asInt());
  //TODO verify that created state here is internally consistent t4155406
  return switchState;
}

SwitchState::SwitchState() {
}

SwitchState::~SwitchState() {
}

void SwitchState::modify(std::shared_ptr<SwitchState>* state) {
  if (!(*state)->isPublished()) {
    return;
  }
  *state = (*state)->clone();
}

std::shared_ptr<Port> SwitchState::getPort(PortID id) const {
  return getFields()->ports->getPort(id);
}

void SwitchState::registerPort(PortID id, const std::string& name) {
  writableFields()->ports->registerPort(id, name);
}

void SwitchState::resetPorts(std::shared_ptr<PortMap> ports) {
  writableFields()->ports.swap(ports);
}

void SwitchState::resetVlans(std::shared_ptr<VlanMap> vlans) {
  writableFields()->vlans.swap(vlans);
}

void SwitchState::addVlan(const std::shared_ptr<Vlan>& vlan) {
  auto* fields = writableFields();
  // For ease-of-use, automatically clone the VlanMap if we are still
  // pointing to a published map.
  if (fields->vlans->isPublished()) {
    fields->vlans = fields->vlans->clone();
  }
  fields->vlans->addVlan(vlan);
}

void SwitchState::setDefaultVlan(VlanID id) {
  writableFields()->defaultVlan = id;
}

void SwitchState::setArpTimeout(std::chrono::seconds timeout) {
  writableFields()->arpTimeout = timeout;
}

void SwitchState::setNdpTimeout(std::chrono::seconds timeout) {
  writableFields()->ndpTimeout = timeout;
}

void SwitchState::setArpAgerInterval(std::chrono::seconds interval) {
  writableFields()->arpAgerInterval = interval;
}

void SwitchState::addIntf(const std::shared_ptr<Interface>& intf) {
  auto* fields = writableFields();
  // For ease-of-use, automatically clone the InterfaceMap if we are still
  // pointing to a published map.
  if (fields->interfaces->isPublished()) {
    fields->interfaces = fields->interfaces->clone();
  }
  fields->interfaces->addInterface(intf);
}

void SwitchState::resetIntfs(std::shared_ptr<InterfaceMap> intfs) {
  writableFields()->interfaces.swap(intfs);
}

void SwitchState::addRouteTable(const std::shared_ptr<RouteTable>& rt) {
  writableFields()->routeTables->addRouteTable(rt);
}

void SwitchState::resetRouteTables(std::shared_ptr<RouteTableMap> rts) {
  writableFields()->routeTables.swap(rts);
}

template class NodeBaseT<SwitchState, SwitchStateFields>;

}} // facebook::fboss
