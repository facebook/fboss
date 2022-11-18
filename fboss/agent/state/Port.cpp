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

state::RxSak PortFields::rxSakToThrift(
    const PortFields::MKASakKey& sakKey,
    const mka::MKASak& sak) const {
  state::MKASakKey sakKeyThrift;
  *sakKeyThrift.sci() = sakKey.sci;
  *sakKeyThrift.associationNum() = sakKey.associationNum;
  state::RxSak rxSakThrift;
  *rxSakThrift.sakKey() = sakKeyThrift;
  *rxSakThrift.sak() = sak;
  return rxSakThrift;
}

std::pair<PortFields::MKASakKey, mka::MKASak> PortFields::rxSakFromThrift(
    state::RxSak rxSak) {
  PortFields::MKASakKey sakKey{
      *rxSak.sakKey()->sci(), *rxSak.sakKey()->associationNum()};
  return std::make_pair(sakKey, *rxSak.sak());
}

state::VlanInfo PortFields::VlanInfo::toThrift() const {
  state::VlanInfo vlanThrift;
  *vlanThrift.tagged() = tagged;
  return vlanThrift;
}

// static
PortFields::VlanInfo PortFields::VlanInfo::fromThrift(
    const state::VlanInfo& vlanThrift) {
  return VlanInfo(*vlanThrift.tagged());
}

// static
PortFields PortFields::fromThrift(state::PortFields const& portThrift) {
  PortFields port(PortID(*portThrift.portId()), *portThrift.portName());
  port.description = *portThrift.portDescription();

  cfg::PortState portState;
  if (!TEnumTraits<cfg::PortState>::findValue(
          portThrift.portState()->c_str(), &portState)) {
    CHECK(false) << "Invalid port state: " << *portThrift.portState();
  }
  port.adminState = portState;

  port.operState = OperState(*portThrift.portOperState());
  port.ingressVlan = VlanID(*portThrift.ingressVlan());

  cfg::PortSpeed portSpeed;
  if (!TEnumTraits<cfg::PortSpeed>::findValue(
          portThrift.portSpeed()->c_str(), &portSpeed)) {
    CHECK(false) << "Invalid port speed: " << *portThrift.portSpeed();
  }
  port.speed = portSpeed;

  if (portThrift.portProfileID()->empty()) {
    // warm booting from a previous version that didn't have profileID set
    XLOG(WARNING) << "Port:" << port.name
                  << " doesn't have portProfileID, set to default.";
    port.profileID = cfg::PortProfileID::PROFILE_DEFAULT;
  } else {
    cfg::PortProfileID portProfileID;
    if (!TEnumTraits<cfg::PortProfileID>::findValue(
            portThrift.portProfileID()->c_str(), &portProfileID)) {
      CHECK(false) << "Invalid port profile id: "
                   << *portThrift.portProfileID();
    }
    port.profileID = portProfileID;
  }

  if (portThrift.portLoopbackMode()->empty()) {
    // Backward compatibility for when we were not serializing loopback mode
    port.loopbackMode = cfg::PortLoopbackMode::NONE;
  } else {
    cfg::PortLoopbackMode portLoopbackMode;
    if (!TEnumTraits<cfg::PortLoopbackMode>::findValue(
            portThrift.portLoopbackMode()->c_str(), &portLoopbackMode)) {
      CHECK(false) << "Unexpected loopback mode value: "
                   << *portThrift.portLoopbackMode();
    }
    port.loopbackMode = portLoopbackMode;
  }

  if (portThrift.sampleDest()) {
    cfg::SampleDestination sampleDest;
    if (!TEnumTraits<cfg::SampleDestination>::findValue(
            portThrift.sampleDest().value().c_str(), &sampleDest)) {
      CHECK(false) << "Unexpected sample destination value: "
                   << portThrift.sampleDest().value();
    }
    port.sampleDest = sampleDest;
  }

  *port.pause.tx() = *portThrift.txPause();
  *port.pause.rx() = *portThrift.rxPause();

  for (const auto& vlanInfo : *portThrift.vlanMemberShips()) {
    port.vlans.emplace(
        VlanID(to<uint32_t>(vlanInfo.first)),
        VlanInfo::fromThrift(vlanInfo.second));
  }

  port.sFlowIngressRate = *portThrift.sFlowIngressRate();
  port.sFlowEgressRate = *portThrift.sFlowEgressRate();

  for (const auto& queue : *portThrift.queues()) {
    port.queues.push_back(std::make_shared<PortQueue>(queue));
  }

  if (auto pgConfigs = portThrift.pgConfigs()) {
    std::vector<PfcPriority> tmpPfcPriorities;
    PortPgConfigs tmpPgConfigs;
    for (const auto& pgConfig : pgConfigs.value()) {
      tmpPgConfigs.push_back(
          std::make_shared<PortPgConfig>(PortPgFields::fromThrift(pgConfig)));
      tmpPfcPriorities.push_back(static_cast<PfcPriority>(*pgConfig.id()));
    }
    port.pgConfigs = tmpPgConfigs;
    port.pfcPriorities = tmpPfcPriorities;
  }

  if (portThrift.ingressMirror()) {
    port.ingressMirror = portThrift.ingressMirror().value();
  }
  if (portThrift.egressMirror()) {
    port.egressMirror = portThrift.egressMirror().value();
  }
  if (portThrift.qosPolicy()) {
    port.qosPolicy = portThrift.qosPolicy().value();
  }
  port.maxFrameSize = *portThrift.maxFrameSize();

  port.lookupClassesToDistrubuteTrafficOn =
      *portThrift.lookupClassesToDistrubuteTrafficOn();

  if (portThrift.pfc()) {
    port.pfc = portThrift.pfc().value();
  }

  if (auto profileCfg = portThrift.profileConfig()) {
    port.profileConfig = *profileCfg;
  }
  if (auto pinCfgs = portThrift.pinConfigs()) {
    port.pinConfigs = *pinCfgs;
  }
  if (auto lineProfileCfg = portThrift.lineProfileConfig()) {
    port.lineProfileConfig = *lineProfileCfg;
  }
  if (auto linePinCfgs = portThrift.linePinConfigs()) {
    port.linePinConfigs = *linePinCfgs;
  }

  port.portType = *portThrift.portType();

  if (auto iPhyLinkFaultStatus = portThrift.iPhyLinkFaultStatus()) {
    port.iPhyLinkFaultStatus = *iPhyLinkFaultStatus;
  }

  port.asicPrbs = *portThrift.asicPrbs();
  port.gbSystemPrbs = *portThrift.gbSystemPrbs();
  port.gbLinePrbs = *portThrift.gbLinePrbs();

  if (auto pfcPriorities = portThrift.pfcPriorities()) {
    std::vector<PfcPriority> tmpPriorities;
    for (const auto& priority : *pfcPriorities) {
      tmpPriorities.push_back(static_cast<PfcPriority>(priority));
    }
    port.pfcPriorities = tmpPriorities;
  }

  port.expectedLLDPValues = *portThrift.expectedLLDPValues();

  for (const auto& rxSak : *portThrift.rxSecureAssociationKeys()) {
    port.rxSecureAssociationKeys.emplace(rxSakFromThrift(rxSak));
  }

  if (auto txSecureAssociationKey = portThrift.txSecureAssociationKey()) {
    port.txSecureAssociationKey = *txSecureAssociationKey;
  }
  port.macsecDesired = *portThrift.macsecDesired();
  port.dropUnencrypted = *portThrift.dropUnencrypted();
  return port;
}

state::PortFields PortFields::toThrift() const {
  state::PortFields port;

  *port.portId() = id;
  *port.portName() = name;
  *port.portDescription() = description;

  // TODO: store admin state as enum, not string?
  auto adminStateName = apache::thrift::util::enumName(adminState);
  if (adminStateName == nullptr) {
    CHECK(false) << "Unexpected admin state: " << static_cast<int>(adminState);
  }
  *port.portState() = adminStateName;

  *port.portOperState() = operState == OperState::UP;
  *port.ingressVlan() = ingressVlan;

  // TODO: store speed as enum, not string?
  auto speedName = apache::thrift::util::enumName(speed);
  if (speedName == nullptr) {
    CHECK(false) << "Unexpected port speed: " << static_cast<int>(speed);
  }
  *port.portSpeed() = speedName;

  *port.portProfileID() = apache::thrift::util::enumNameSafe(profileID);

  auto loopbackModeName = apache::thrift::util::enumName(loopbackMode);
  if (loopbackModeName == nullptr) {
    CHECK(false) << "Unexpected port LoopbackMode: "
                 << static_cast<int>(loopbackMode);
  }
  *port.portLoopbackMode() = loopbackModeName;

  *port.txPause() = *pause.tx();
  *port.rxPause() = *pause.rx();

  for (const auto& vlan : vlans) {
    port.vlanMemberShips()[to<string>(vlan.first)] = vlan.second.toThrift();
  }

  *port.sFlowIngressRate() = sFlowIngressRate;
  *port.sFlowEgressRate() = sFlowEgressRate;

  for (const auto& queue : queues) {
    port.queues()->push_back(queue->toThrift());
  }

  if (pgConfigs) {
    std::vector<state::PortPgFields> tmpPgConfigs;
    for (const auto& pgConfig : *pgConfigs) {
      tmpPgConfigs.emplace_back(pgConfig->toThrift());
    }
    port.pgConfigs() = tmpPgConfigs;
  }

  if (ingressMirror) {
    port.ingressMirror() = ingressMirror.value();
  }
  if (egressMirror) {
    port.egressMirror() = egressMirror.value();
  }
  if (qosPolicy) {
    port.qosPolicy() = qosPolicy.value();
  }

  if (sampleDest) {
    port.sampleDest() = apache::thrift::util::enumName(sampleDest.value());
  }

  *port.lookupClassesToDistrubuteTrafficOn() =
      lookupClassesToDistrubuteTrafficOn;
  *port.maxFrameSize() = maxFrameSize;

  if (pfc) {
    port.pfc() = pfc.value();
  }

  port.profileConfig() = profileConfig;
  port.pinConfigs() = pinConfigs;
  if (lineProfileConfig) {
    port.lineProfileConfig() = *lineProfileConfig;
  }
  if (linePinConfigs) {
    port.linePinConfigs() = *linePinConfigs;
  }

  *port.portType() = portType;

  if (iPhyLinkFaultStatus) {
    port.iPhyLinkFaultStatus() = *iPhyLinkFaultStatus;
  }

  *port.asicPrbs() = asicPrbs;
  *port.gbSystemPrbs() = gbSystemPrbs;
  *port.gbLinePrbs() = gbLinePrbs;

  if (pfcPriorities) {
    std::vector<int16_t> tmpPriorities;
    for (const auto& priority : *pfcPriorities) {
      tmpPriorities.push_back(priority);
    }
    port.pfcPriorities() = tmpPriorities;
  }

  *port.expectedLLDPValues() = expectedLLDPValues;

  for (const auto& [mkaSakKey, mkaSak] : rxSecureAssociationKeys) {
    port.rxSecureAssociationKeys()->push_back(rxSakToThrift(mkaSakKey, mkaSak));
  }

  if (txSecureAssociationKey) {
    port.txSecureAssociationKey() = *txSecureAssociationKey;
  }
  *port.macsecDesired() = macsecDesired;
  *port.dropUnencrypted() = dropUnencrypted;
  return port;
}

bool PortFields::operator==(const PortFields& other) const {
  return id == other.id && name == other.name &&
      description == other.description && adminState == other.adminState &&
      operState == other.operState && asicPrbs == other.asicPrbs &&
      gbSystemPrbs == other.gbSystemPrbs && gbLinePrbs == other.gbLinePrbs &&
      ingressVlan == other.ingressVlan && speed == other.speed &&
      pause == other.pause && pfc == other.pfc &&
      pfcPriorities == other.pfcPriorities && vlans == other.vlans &&
      sFlowIngressRate == other.sFlowIngressRate &&
      sFlowEgressRate == other.sFlowEgressRate &&
      sampleDest == other.sampleDest && loopbackMode == other.loopbackMode &&
      ingressMirror == other.ingressMirror &&
      egressMirror == other.egressMirror && qosPolicy == other.qosPolicy &&
      expectedLLDPValues == other.expectedLLDPValues &&
      lookupClassesToDistrubuteTrafficOn ==
      other.lookupClassesToDistrubuteTrafficOn &&
      profileID == other.profileID && maxFrameSize == other.maxFrameSize &&
      ThriftyUtils::listEq(queues, other.queues) &&
      ThriftyUtils::listEq(pgConfigs, other.pgConfigs) &&
      portType == other.portType;
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

void Port::fillPhyInfo(phy::PhyInfo* phyInfo) {
  phyInfo->name() = getName();
  phyInfo->speed() = getSpeed();
}

template class NodeBaseT<Port, PortFields>;

} // namespace facebook::fboss
