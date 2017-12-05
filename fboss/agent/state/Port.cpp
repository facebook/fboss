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
#include "fboss/agent/state/PortQueue.h"
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
constexpr auto kPortTxPause = "txPause";
constexpr auto kPortRxPause = "rxPause";
constexpr auto kVlanMemberships = "vlanMemberShips";
constexpr auto kVlanId = "vlanId";
constexpr auto kVlanInfo = "vlanInfo";
constexpr auto kTagged = "tagged";
constexpr auto kSflowIngressRate = "sFlowIngressRate";
constexpr auto kSflowEgressRate = "sFlowEgressRate";
constexpr auto kQueues = "queues";
constexpr auto kPortFEC = "portFEC";
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
  auto itrAdminState  = cfg::_PortState_VALUES_TO_NAMES.find(adminState);
  CHECK(itrAdminState != cfg::_PortState_VALUES_TO_NAMES.end());
  port[kPortState] = itrAdminState->second;
  port[kPortOperState] = operState == OperState::UP;
  port[kIngressVlan] = static_cast<uint16_t>(ingressVlan);
  auto itr_speed  = cfg::_PortSpeed_VALUES_TO_NAMES.find(speed);
  CHECK(itr_speed != cfg::_PortSpeed_VALUES_TO_NAMES.end());
  port[kPortSpeed] = itr_speed->second;

  // needed for backwards compatibility
  // TODO(aeckert): t24117229 remove this after next version is pushed
  port["portMaxSpeed"] = itr_speed->second;

  auto itr_port_fec  = cfg::_PortFEC_VALUES_TO_NAMES.find(fec);
  CHECK(itr_port_fec != cfg::_PortFEC_VALUES_TO_NAMES.end())
     << "Unexpected port FEC: " << static_cast<int>(fec);
  port[kPortFEC] = itr_port_fec->second;
  port[kPortTxPause] = pause.tx;
  port[kPortRxPause] = pause.rx;
  port[kVlanMemberships] = folly::dynamic::object;
  for (const auto& vlan: vlans) {
    port[kVlanMemberships][to<string>(vlan.first)] =
      vlan.second.toFollyDynamic();
  }
  port[kSflowIngressRate] = sFlowIngressRate;
  port[kSflowEgressRate] = sFlowEgressRate;
  port[kQueues] = folly::dynamic::array;
  for (const auto& queue : queues) {
    port[kQueues].push_back(queue.second->toFollyDynamic());
  }
  return port;
}

PortFields PortFields::fromFollyDynamic(const folly::dynamic& portJson) {
  PortFields port(PortID(portJson[kPortId].asInt()),
      portJson[kPortName].asString());
  port.description = portJson[kPortDescription].asString();
  // For backwards compatibility, we still need the ability to read in
  // both possible names for the admin port state. The production agent
  // still writes out POWER_DOWN/UP instead of DISABLED/ENABLED. After
  // another release, where we only write ENABLED/DISABLED we can get rid
  // of this and use the NAMES_TO_VALUES map directly.
  std::unordered_map<std::string, cfg::PortState> transitionAdminStateMap{
      {"DISABLED", cfg::PortState::DISABLED},
      {"POWER_DOWN", cfg::PortState::DISABLED},
      {"DOWN", cfg::PortState::DISABLED},
      {"ENABLED", cfg::PortState::ENABLED},
      {"UP", cfg::PortState::ENABLED},
  };
  auto itrAdminState = transitionAdminStateMap.find(
    portJson[kPortState].asString());
  CHECK(itrAdminState != transitionAdminStateMap.end());
  port.adminState = itrAdminState->second;
  port.operState =
      OperState(portJson.getDefault(kPortOperState, false).asBool());
  port.ingressVlan = VlanID(portJson[kIngressVlan].asInt());
  auto itr_speed  = cfg::_PortSpeed_NAMES_TO_VALUES.find(
      portJson[kPortSpeed].asString().c_str());
  CHECK(itr_speed != cfg::_PortSpeed_NAMES_TO_VALUES.end());
  port.speed = cfg::PortSpeed(itr_speed->second);
  auto itr_port_fec = cfg::_PortFEC_NAMES_TO_VALUES.find(
    portJson.getDefault(kPortFEC, "OFF").asString().c_str());
  CHECK(itr_port_fec != cfg::_PortFEC_NAMES_TO_VALUES.end());
  port.fec = cfg::PortFEC(itr_port_fec->second);
  auto tx_pause_itr = portJson.find(kPortTxPause);
  if (tx_pause_itr != portJson.items().end()) {
    port.pause.tx = tx_pause_itr->second.asBool();
  }
  auto rx_pause_itr = portJson.find(kPortRxPause);
  if (rx_pause_itr != portJson.items().end()) {
    port.pause.tx = rx_pause_itr->second.asBool();
  }
  for (const auto& vlanInfo: portJson[kVlanMemberships].items()) {
    port.vlans.emplace(VlanID(to<uint32_t>(vlanInfo.first.asString())),
      VlanInfo::fromFollyDynamic(vlanInfo.second));
  }
  if (portJson.count(kSflowIngressRate) > 0) {
    port.sFlowIngressRate = portJson[kSflowIngressRate].asInt();
  }
  if (portJson.count(kSflowEgressRate) > 0) {
    port.sFlowEgressRate = portJson[kSflowEgressRate].asInt();
  }
  if (portJson.find(kQueues) != portJson.items().end()) {
    for (const auto& queue : portJson[kQueues]) {
      auto madeQueue = PortQueue::fromFollyDynamic(queue);
      port.queues.emplace(std::make_pair(madeQueue->getID(), madeQueue));
    }
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
