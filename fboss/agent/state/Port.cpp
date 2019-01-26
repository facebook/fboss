/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "Port.h"

#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include <folly/Conv.h>

#include "fboss/agent/state/NodeBase-defs.h"

using folly::to;
using std::string;

namespace facebook { namespace fboss {


state::VlanInfo PortFields::VlanInfo::toThrift() const {
  state::VlanInfo vlanThrift;
  vlanThrift.tagged = tagged;
  return vlanThrift;
}

// static
PortFields::VlanInfo
PortFields::VlanInfo::fromThrift(const state::VlanInfo& vlanThrift) {
  return VlanInfo(vlanThrift.tagged);
}

// static
PortFields PortFields::fromThrift(state::PortFields const& portThrift) {
  PortFields port(PortID(portThrift.portId), portThrift.portName);
  port.description = portThrift.portDescription;

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
  auto itrAdminState = transitionAdminStateMap.find(portThrift.portState);
  CHECK(itrAdminState != transitionAdminStateMap.end())
      << "Invalid port state: " << portThrift.portState;
  port.adminState = itrAdminState->second;

  port.operState = OperState(portThrift.portOperState);
  port.ingressVlan = VlanID(portThrift.ingressVlan);

  auto itrSpeed =
      cfg::_PortSpeed_NAMES_TO_VALUES.find(portThrift.portSpeed.c_str());
  CHECK(itrSpeed != cfg::_PortSpeed_NAMES_TO_VALUES.end())
      << "Invalid port speed: " << portThrift.portSpeed;
  port.speed = cfg::PortSpeed(itrSpeed->second);

  auto itrPortFec =
      cfg::_PortFEC_NAMES_TO_VALUES.find(portThrift.portFEC.c_str());
  CHECK(itrPortFec != cfg::_PortFEC_NAMES_TO_VALUES.end())
      << "Unexpected FEC value: " << portThrift.portFEC;
  port.fec = cfg::PortFEC(itrPortFec->second);

  if (portThrift.portLoopbackMode.empty()) {
    // Backward compatibility for when we were not serializing loopback mode
    port.loopbackMode = cfg::PortLoopbackMode::NONE;
  } else {
    auto itrPortLoopbackMode = cfg::_PortLoopbackMode_NAMES_TO_VALUES.find(
        portThrift.portLoopbackMode.c_str());
    CHECK(itrPortLoopbackMode != cfg::_PortLoopbackMode_NAMES_TO_VALUES.end())
      << "Unexpected loopback mode value: " << portThrift.portLoopbackMode;
    port.loopbackMode = cfg::PortLoopbackMode(itrPortLoopbackMode->second);
  }

  port.pause.tx = portThrift.txPause;
  port.pause.rx = portThrift.rxPause;

  for (const auto& vlanInfo : portThrift.vlanMemberShips) {
    port.vlans.emplace(
        VlanID(to<uint32_t>(vlanInfo.first)),
        VlanInfo::fromThrift(vlanInfo.second));
  }

  port.sFlowIngressRate = portThrift.sFlowIngressRate;
  port.sFlowEgressRate = portThrift.sFlowEgressRate;

  for (const auto& queue : portThrift.queues) {
    port.queues.push_back(
        std::make_shared<PortQueue>(PortQueueFields::fromThrift(queue)));
  }

  if (portThrift.ingressMirror) {
    port.ingressMirror = portThrift.ingressMirror;
  }
  if (portThrift.egressMirror) {
    port.egressMirror = portThrift.egressMirror;
  }
  port.qosPolicy.assign(portThrift.qosPolicy);

  return port;
}

state::PortFields PortFields::toThrift() const {
  state::PortFields port;

  port.portId = id;
  port.portName = name;
  port.portDescription = description;

  // TODO: store admin state as enum, not string?
  auto itrAdminState  = cfg::_PortState_VALUES_TO_NAMES.find(adminState);
  CHECK(itrAdminState != cfg::_PortState_VALUES_TO_NAMES.end())
      << "Unexpected admin state: " << static_cast<int>(adminState);
  port.portState = itrAdminState->second;

  port.portOperState = operState == OperState::UP;
  port.ingressVlan = ingressVlan;

  // TODO: store speed as enum, not string?
  auto itrSpeed  = cfg::_PortSpeed_VALUES_TO_NAMES.find(speed);
  CHECK(itrSpeed != cfg::_PortSpeed_VALUES_TO_NAMES.end())
      << "Unexpected port speed: " << static_cast<int>(speed);
  port.portSpeed = itrSpeed->second;

  // TODO(aeckert): t24117229 remove this after next version is pushed
  port.portMaxSpeed = port.portSpeed;

  auto itrPortFec  = cfg::_PortFEC_VALUES_TO_NAMES.find(fec);
  CHECK(itrPortFec != cfg::_PortFEC_VALUES_TO_NAMES.end())
      << "Unexpected port FEC: " << static_cast<int>(fec);
  port.portFEC = itrPortFec->second;

  auto itrPortLoopbackMode =
      cfg::_PortLoopbackMode_VALUES_TO_NAMES.find(loopbackMode);
  CHECK(itrPortLoopbackMode != cfg::_PortLoopbackMode_VALUES_TO_NAMES.end())
      << "Unexpected port LoopbackMode: " << static_cast<int>(loopbackMode);
  port.portLoopbackMode = itrPortLoopbackMode->second;

  port.txPause = pause.tx;
  port.rxPause = pause.rx;

  for (const auto& vlan: vlans) {
    port.vlanMemberShips[to<string>(vlan.first)] = vlan.second.toThrift();
  }

  port.sFlowIngressRate = sFlowIngressRate;
  port.sFlowEgressRate = sFlowEgressRate;

  for (const auto& queue : queues) {
    // TODO: Use PortQueue::toThrift() when available
    port.queues.push_back(queue->getFields()->toThrift());
  }

  if (ingressMirror) {
    port.ingressMirror = ingressMirror;
  }
  if (egressMirror) {
    port.egressMirror = egressMirror;
  }
  port.qosPolicy.assign(qosPolicy);

  return port;
}

Port::Port(PortID id, const std::string& name)
  : ThriftyBaseT(id, name) {
}

void Port::initDefaultConfigState(cfg::Port* config) const {
  // Copy over port identifiers and reset to (default)
  // admin disabled state.
  config->logicalID = getID();
  config->name_ref().value_unchecked() = getName();
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
