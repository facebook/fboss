/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include <folly/Conv.h>

#include "fboss/agent/state/NodeBase-defs.h"

using folly::to;
using std::string;

namespace {
constexpr auto kPortId = "portId";
constexpr auto kPortName = "portName";
constexpr auto kPortDescription = "portDescription";
constexpr auto kPortState = "portState";
constexpr auto kPortOperState = "portOperState";
constexpr auto kIngressVlan = "ingressVlan";
constexpr auto kPortSpeed = "portSpeed";
constexpr auto kPortMaxSpeed = "portMaxSpeed";
constexpr auto kVlanMemberships = "vlanMemberShips";
constexpr auto kVlanId = "vlanId";
constexpr auto kVlanInfo = "vlanInfo";
constexpr auto kTagged = "tagged";
}
namespace facebook { namespace fboss {


folly::dynamic PortFields::VlanInfo::toFollyDynamic() const {
  folly::dynamic vlanInfo = folly::dynamic::object;
  vlanInfo[kTagged] = tagged;
  return vlanInfo;
}

PortFields::VlanInfo
PortFields::VlanInfo::fromFollyDynamic(const folly::dynamic& json) {
  return VlanInfo(json[kTagged].asBool());
}

folly::dynamic PortFields::toFollyDynamic() const {
  folly::dynamic port = folly::dynamic::object;
  port[kPortId] = static_cast<uint16_t>(id);
  port[kPortName] = name;
  port[kPortDescription] = description;
  // For backwards compatibility, we must keep writing out the old strings
  // otherwise, if we were to try to roll back with a warm boot, the old
  // agent would crash failing to find "ENABLED" or "DISABLED" in
  // the VALUES_TO_NAMES map. After the agent everywhere is capable of
  // reading in either UP/POWER_DOWN or ENABLED/DISABLED,
  // then we can remove this.
  // Finally, after a release like that, we can remove the code that reads in
  // "POWER_DOWN" and "UP".
  std::unordered_map<cfg::PortState, std::string> transitionAdminStateMap{
      {cfg::PortState::DISABLED, "POWER_DOWN"},
      {cfg::PortState::ENABLED, "UP"},
      // There should be no instances of DOWN, but leave it in the map just
      // in case to avoid crashes if we missed deprecating it somewhere.
      {cfg::PortState::DOWN, "POWER_DOWN"},
  };
  auto itrAdminState  = transitionAdminStateMap.find(adminState);
  CHECK(itrAdminState != transitionAdminStateMap.end());
  port[kPortState] = itrAdminState->second;
  port[kPortOperState] = operState == OperState::UP;
  port[kIngressVlan] = static_cast<uint16_t>(ingressVlan);
  auto itr_speed  = cfg::_PortSpeed_VALUES_TO_NAMES.find(speed);
  CHECK(itr_speed != cfg::_PortSpeed_VALUES_TO_NAMES.end());
  port[kPortSpeed] = itr_speed->second;
  auto itr_max_speed  = cfg::_PortSpeed_VALUES_TO_NAMES.find(maxSpeed);
  CHECK(itr_max_speed != cfg::_PortSpeed_VALUES_TO_NAMES.end())
     << "Unexpected max speed: " << static_cast<int>(maxSpeed);
  port[kPortMaxSpeed] = itr_max_speed->second;
  port[kVlanMemberships] = folly::dynamic::object;
  for (const auto& vlan: vlans) {
    port[kVlanMemberships][to<string>(vlan.first)] =
      vlan.second.toFollyDynamic();
  }
  return port;
}

PortFields PortFields::fromFollyDynamic(const folly::dynamic& portJson) {
  PortFields port(PortID(portJson[kPortId].asInt()),
      portJson[kPortName].asString());
  port.description = portJson[kPortDescription].asString();
  // see note in toFollyDynamic for an explanation of the ugprade path
  // to get rid of this backwards compatibility hack.
  std::unordered_map<std::string, cfg::PortState> transitionAdminStateMap{
      {"DISABLED", cfg::PortState::DISABLED},
      {"POWER_DOWN", cfg::PortState::DISABLED},
      {"DOWN", cfg::PortState::DISABLED},
      {"ENABLED", cfg::PortState::ENABLED},
      {"UP", cfg::PortState::ENABLED},
  };
  auto itrAdminState = transitionAdminStateMap.find(
      util::getCpp2EnumName(portJson[kPortState].asString()).c_str());
  CHECK(itrAdminState != transitionAdminStateMap.end());
  port.adminState = itrAdminState->second;
  port.operState =
      OperState(portJson.getDefault(kPortOperState, false).asBool());
  port.ingressVlan = VlanID(portJson[kIngressVlan].asInt());
  auto itr_speed  = cfg::_PortSpeed_NAMES_TO_VALUES.find(
      util::getCpp2EnumName(portJson[kPortSpeed].asString()).c_str());
  CHECK(itr_speed != cfg::_PortSpeed_NAMES_TO_VALUES.end());
  port.speed = cfg::PortSpeed(itr_speed->second);
  auto itr_max_speed  = cfg::_PortSpeed_NAMES_TO_VALUES.find(
      util::getCpp2EnumName(portJson[kPortMaxSpeed].asString()).c_str());
  CHECK(itr_max_speed != cfg::_PortSpeed_NAMES_TO_VALUES.end());
  port.maxSpeed = cfg::PortSpeed(itr_max_speed->second);
  for (const auto& vlanInfo: portJson[kVlanMemberships].items()) {
    port.vlans.emplace(VlanID(to<uint32_t>(vlanInfo.first.asString())),
      VlanInfo::fromFollyDynamic(vlanInfo.second));
  }
  return port;
}

Port::Port(PortID id, const std::string& name)
  : NodeBaseT(id, name) {
}

void Port::initDefaultConfigState(cfg::Port* config) const {
  // Copy over port identifiers and reset to (default)
  // admin disabled state.
  config->logicalID = getID();
  config->name = getName();
  config->state = cfg::PortState::DISABLED;
}

Port* Port::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  PortMap* ports = (*state)->getPorts()->modify(state);
  auto newPort = clone();
  auto* ptr = newPort.get();
  ports->updatePort(std::move(newPort));
  return ptr;
}

template class NodeBaseT<Port, PortFields>;

}} // facebook::fboss
