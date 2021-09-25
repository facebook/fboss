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
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/PortPgConfig.h"
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

  cfg::PortState portState;
  if (!TEnumTraits<cfg::PortState>::findValue(
          portThrift.portState_ref()->c_str(), &portState)) {
    CHECK(false) << "Invalid port state: " << *portThrift.portState_ref();
  }
  port.adminState = portState;

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

  if (auto pgConfigs = portThrift.pgConfigs_ref()) {
    std::vector<PfcPriority> tmpPfcPriorities;
    PortPgConfigs tmpPgConfigs;
    for (const auto& pgConfig : pgConfigs.value()) {
      tmpPgConfigs.push_back(
          std::make_shared<PortPgConfig>(PortPgFields::fromThrift(pgConfig)));
      tmpPfcPriorities.push_back(static_cast<PfcPriority>(*pgConfig.id_ref()));
    }
    port.pgConfigs = tmpPgConfigs;
    port.pfcPriorities = tmpPfcPriorities;
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

  if (portThrift.pfc_ref()) {
    port.pfc = portThrift.pfc_ref().value();
  }

  if (auto profileCfg = portThrift.profileConfig_ref()) {
    port.profileConfig = *profileCfg;
  }
  if (auto pinCfgs = portThrift.pinConfigs_ref()) {
    port.pinConfigs = *pinCfgs;
  }

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

  if (pgConfigs) {
    std::vector<state::PortPgFields> tmpPgConfigs;
    for (const auto& pgConfig : *pgConfigs) {
      tmpPgConfigs.emplace_back(pgConfig->getFields()->toThrift());
    }
    port.pgConfigs_ref() = tmpPgConfigs;
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

  if (pfc) {
    port.pfc_ref() = pfc.value();
  }

  port.profileConfig_ref() = profileConfig;
  port.pinConfigs_ref() = pinConfigs;

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
