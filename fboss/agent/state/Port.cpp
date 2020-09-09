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

#include <folly/Conv.h>
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

using apache::thrift::TEnumTraits;
using folly::to;
using std::string;

namespace facebook::fboss {

state::VlanInfo PortFields::VlanInfo::toThrift() const {
  state::VlanInfo vlanThrift;
  *vlanThrift.tagged_ref() = tagged;
  return vlanThrift;
}

// static
PortFields::VlanInfo PortFields::VlanInfo::fromThrift(
    const state::VlanInfo& vlanThrift) {
  return VlanInfo(*vlanThrift.tagged_ref());
}

// static
PortFields PortFields::fromThrift(state::PortFields const& portThrift) {
  PortFields port(PortID(portThrift.portId), portThrift.portName);
  port.description = *portThrift.portDescription_ref();

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
  auto itrAdminState =
      transitionAdminStateMap.find(*portThrift.portState_ref());
  CHECK(itrAdminState != transitionAdminStateMap.end())
      << "Invalid port state: " << *portThrift.portState_ref();
  port.adminState = itrAdminState->second;

  port.operState = OperState(*portThrift.portOperState_ref());
  port.ingressVlan = VlanID(*portThrift.ingressVlan_ref());

  cfg::PortSpeed portSpeed;
  if (!TEnumTraits<cfg::PortSpeed>::findValue(
          portThrift.portSpeed_ref()->c_str(), &portSpeed)) {
    CHECK(false) << "Invalid port speed: " << *portThrift.portSpeed_ref();
  }
  port.speed = portSpeed;

  if (portThrift.portProfileID_ref()->empty()) {
    // warm booting from a previous version that didn't have profileID set
    XLOG(WARNING) << "Port:" << port.name
                  << " doesn't have portProfileID, set to default.";
    port.profileID = cfg::PortProfileID::PROFILE_DEFAULT;
  } else {
    cfg::PortProfileID portProfileID;
    if (!TEnumTraits<cfg::PortProfileID>::findValue(
            portThrift.portProfileID_ref()->c_str(), &portProfileID)) {
      CHECK(false) << "Invalid port profile id: "
                   << *portThrift.portProfileID_ref();
    }
    port.profileID = portProfileID;
  }

  cfg::PortFEC portFEC;
  if (!TEnumTraits<cfg::PortFEC>::findValue(
          portThrift.portFEC_ref()->c_str(), &portFEC)) {
    CHECK(false) << "Unexpected FEC value: " << *portThrift.portFEC_ref();
  }
  port.fec = portFEC;

  if (portThrift.portLoopbackMode_ref()->empty()) {
    // Backward compatibility for when we were not serializing loopback mode
    port.loopbackMode = cfg::PortLoopbackMode::NONE;
  } else {
    cfg::PortLoopbackMode portLoopbackMode;
    if (!TEnumTraits<cfg::PortLoopbackMode>::findValue(
            portThrift.portLoopbackMode_ref()->c_str(), &portLoopbackMode)) {
      CHECK(false) << "Unexpected loopback mode value: "
                   << *portThrift.portLoopbackMode_ref();
    }
    port.loopbackMode = portLoopbackMode;
  }

  if (portThrift.sampleDest_ref()) {
    cfg::SampleDestination sampleDest;
    if (!TEnumTraits<cfg::SampleDestination>::findValue(
            portThrift.sampleDest_ref().value().c_str(), &sampleDest)) {
      CHECK(false) << "Unexpected sample destination value: "
                   << portThrift.sampleDest_ref().value();
    }
    port.sampleDest = sampleDest;
  }

  *port.pause.tx_ref() = *portThrift.txPause_ref();
  *port.pause.rx_ref() = *portThrift.rxPause_ref();

  for (const auto& vlanInfo : *portThrift.vlanMemberShips_ref()) {
    port.vlans.emplace(
        VlanID(to<uint32_t>(vlanInfo.first)),
        VlanInfo::fromThrift(vlanInfo.second));
  }

  port.sFlowIngressRate = *portThrift.sFlowIngressRate_ref();
  port.sFlowEgressRate = *portThrift.sFlowEgressRate_ref();

  for (const auto& queue : *portThrift.queues_ref()) {
    port.queues.push_back(
        std::make_shared<PortQueue>(PortQueueFields::fromThrift(queue)));
  }

  if (portThrift.ingressMirror_ref()) {
    port.ingressMirror = portThrift.ingressMirror_ref().value();
  }
  if (portThrift.egressMirror_ref()) {
    port.egressMirror = portThrift.egressMirror_ref().value();
  }
  if (portThrift.qosPolicy_ref()) {
    port.qosPolicy = portThrift.qosPolicy_ref().value();
  }
  port.maxFrameSize = *portThrift.maxFrameSize_ref();

  port.lookupClassesToDistrubuteTrafficOn =
      *portThrift.lookupClassesToDistrubuteTrafficOn_ref();

  return port;
}

state::PortFields PortFields::toThrift() const {
  state::PortFields port;

  port.portId = id;
  port.portName = name;
  *port.portDescription_ref() = description;

  // TODO: store admin state as enum, not string?
  auto adminStateName = apache::thrift::util::enumName(adminState);
  if (adminStateName == nullptr) {
    CHECK(false) << "Unexpected admin state: " << static_cast<int>(adminState);
  }
  *port.portState_ref() = adminStateName;

  *port.portOperState_ref() = operState == OperState::UP;
  *port.ingressVlan_ref() = ingressVlan;

  // TODO: store speed as enum, not string?
  auto speedName = apache::thrift::util::enumName(speed);
  if (speedName == nullptr) {
    CHECK(false) << "Unexpected port speed: " << static_cast<int>(speed);
  }
  *port.portSpeed_ref() = speedName;

  *port.portProfileID_ref() = apache::thrift::util::enumNameSafe(profileID);

  // TODO(aeckert): t24117229 remove this after next version is pushed
  *port.portMaxSpeed_ref() = *port.portSpeed_ref();

  auto fecName = apache::thrift::util::enumName(fec);
  if (fecName == nullptr) {
    CHECK(false) << "Unexpected port FEC: " << static_cast<int>(fec);
  }
  *port.portFEC_ref() = fecName;

  auto loopbackModeName = apache::thrift::util::enumName(loopbackMode);
  if (loopbackModeName == nullptr) {
    CHECK(false) << "Unexpected port LoopbackMode: "
                 << static_cast<int>(loopbackMode);
  }
  *port.portLoopbackMode_ref() = loopbackModeName;

  *port.txPause_ref() = *pause.tx_ref();
  *port.rxPause_ref() = *pause.rx_ref();

  for (const auto& vlan : vlans) {
    port.vlanMemberShips_ref()[to<string>(vlan.first)] = vlan.second.toThrift();
  }

  *port.sFlowIngressRate_ref() = sFlowIngressRate;
  *port.sFlowEgressRate_ref() = sFlowEgressRate;

  for (const auto& queue : queues) {
    // TODO: Use PortQueue::toThrift() when available
    port.queues_ref()->push_back(queue->getFields()->toThrift());
  }

  if (ingressMirror) {
    port.ingressMirror_ref() = ingressMirror.value();
  }
  if (egressMirror) {
    port.egressMirror_ref() = egressMirror.value();
  }
  if (qosPolicy) {
    port.qosPolicy_ref() = qosPolicy.value();
  }

  if (sampleDest) {
    port.sampleDest_ref() = apache::thrift::util::enumName(sampleDest.value());
  }

  *port.lookupClassesToDistrubuteTrafficOn_ref() =
      lookupClassesToDistrubuteTrafficOn;
  *port.maxFrameSize_ref() = maxFrameSize;
  return port;
}

Port::Port(PortID id, const std::string& name) : ThriftyBaseT(id, name) {}

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

} // namespace facebook::fboss
