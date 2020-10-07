/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/MacAddress.h>

#include <algorithm>
#include <tuple>
#include <utility>

namespace {
constexpr auto kId = "id";
constexpr auto kName = "name";
constexpr auto kDescription = "description";
constexpr auto kMinimumLinkCount = "minimumLinkCount";
constexpr auto kSubports = "subports";
constexpr auto kSystemID = "systemID";
constexpr auto kSystemPriority = "systemPriority";
constexpr auto kPortID = "portId";
constexpr auto kRate = "rate";
constexpr auto kActivity = "activity";
constexpr auto kPriority = "priority";
constexpr auto kForwarding = "forwarding";
constexpr auto kForwardingStates = "forwardingStates";
constexpr auto kPartnerInfo = "partnerInfo";
constexpr auto kPartnerInfos = "partnerInfos";
constexpr auto kHoldTimerMultiplier = "holdTimerMultiplier";
} // namespace

namespace facebook::fboss {

folly::dynamic AggregatePortFields::Subport::toFollyDynamic() const {
  folly::dynamic subport = folly::dynamic::object;
  subport[kPortID] = static_cast<uint16_t>(portID);
  subport[kPriority] = static_cast<uint16_t>(priority);
  subport[kHoldTimerMultiplier] = static_cast<uint16_t>(holdTimerMulitiplier);
  subport[kRate] = rate == cfg::LacpPortRate::FAST ? "fast" : "slow";
  subport[kActivity] =
      activity == cfg::LacpPortActivity::ACTIVE ? "active" : "passive";
  return subport;
}

folly::dynamic AggregatePortFields::portAndFwdStateToFollyDynamic(
    const std::pair<PortID, Forwarding>& portState) const {
  folly::dynamic state = folly::dynamic::object;
  state[kPortID] = static_cast<uint16_t>(portState.first);
  state[kForwarding] =
      portState.second == Forwarding::ENABLED ? "enabled" : "disabled";
  return state;
}

folly::dynamic AggregatePortFields::portAndPartnerStateToFollyDynamic(
    const std::pair<PortID, ParticipantInfo>& partnerInfo) const {
  const auto& partnerState = partnerInfo.second;
  folly::dynamic portAndPartner = folly::dynamic::object;
  portAndPartner[kPortID] = static_cast<uint16_t>(partnerInfo.first);
  portAndPartner[kPartnerInfo] = partnerState.toFollyDynamic();
  return portAndPartner;
}

AggregatePortFields::Subport AggregatePortFields::Subport::fromFollyDynamic(
    const folly::dynamic& json) {
  cfg::LacpPortRate rate;
  if (json[kRate] == "fast") {
    rate = cfg::LacpPortRate::FAST;
  } else {
    CHECK_EQ(json[kRate], "slow");
    rate = cfg::LacpPortRate::SLOW;
  }

  cfg::LacpPortActivity activity;
  if (json[kActivity] == "active") {
    activity = cfg::LacpPortActivity::ACTIVE;
  } else {
    CHECK_EQ(json[kActivity], "passive");
    activity = cfg::LacpPortActivity::PASSIVE;
  }

  // TODO(samank): check widths match up
  auto id = static_cast<PortID>(json[kPortID].asInt());
  auto priority = static_cast<uint16_t>(json[kPriority].asInt());
  auto timer = static_cast<uint16_t>(json[kHoldTimerMultiplier].asInt());

  return Subport(id, priority, rate, activity, timer);
}

AggregatePortFields::AggregatePortFields(
    AggregatePortID id,
    const std::string& name,
    const std::string& description,
    uint16_t systemPriority,
    folly::MacAddress systemID,
    uint8_t minimumLinkCount,
    Subports&& ports,
    SubportToForwardingState&& portStates,
    SubportToPartnerState&& portPartnerStates)
    : id_(id),
      name_(name),
      description_(description),
      systemPriority_(systemPriority),
      systemID_(systemID),
      minimumLinkCount_(minimumLinkCount),
      ports_(std::move(ports)),
      portToFwdState_(std::move(portStates)),
      portToPartnerState_(std::move(portPartnerStates)) {}

AggregatePortFields::AggregatePortFields(
    AggregatePortID id,
    const std::string& name,
    const std::string& description,
    uint16_t systemPriority,
    folly::MacAddress systemID,
    uint8_t minimumLinkCount,
    Subports&& ports,
    AggregatePortFields::Forwarding fwd,
    ParticipantInfo pState)
    : id_(id),
      name_(name),
      description_(description),
      systemPriority_(systemPriority),
      systemID_(systemID),
      minimumLinkCount_(minimumLinkCount),
      ports_(std::move(ports)),
      portToFwdState_(),
      portToPartnerState_() {
  for (const auto& subport : ports_) {
    auto hint = portToFwdState_.end();
    portToFwdState_.emplace_hint(hint, subport.portID, fwd);
  }
  for (const auto& subport : ports_) {
    auto hint = portToPartnerState_.end();
    portToPartnerState_.emplace_hint(hint, subport.portID, pState);
  }
}

folly::dynamic AggregatePortFields::toFollyDynamic() const {
  folly::dynamic aggPortFields = folly::dynamic::object;
  aggPortFields[kId] = static_cast<uint16_t>(id_);
  aggPortFields[kName] = name_;
  aggPortFields[kDescription] = description_;
  aggPortFields[kMinimumLinkCount] = minimumLinkCount_;

  folly::dynamic subports = folly::dynamic::array;
  for (const auto& subport : ports_) {
    subports.push_back(subport.toFollyDynamic());
  }
  aggPortFields[kSubports] = std::move(subports);
  aggPortFields[kSystemID] = systemID_.toString();
  aggPortFields[kSystemPriority] = static_cast<uint16_t>(systemPriority_);

  folly::dynamic fwdingState = folly::dynamic::array();
  for (const auto& state : portToFwdState_) {
    fwdingState.push_back(portAndFwdStateToFollyDynamic(state));
  }
  aggPortFields[kForwardingStates] = fwdingState;

  folly::dynamic partnerState = folly::dynamic::array();
  for (const auto& pState : portToPartnerState_) {
    partnerState.push_back(portAndPartnerStateToFollyDynamic(pState));
  }
  aggPortFields[kPartnerInfos] = partnerState;
  return aggPortFields;
}

AggregatePortFields AggregatePortFields::fromFollyDynamic(
    const folly::dynamic& json) {
  Subports ports;

  ports.reserve(json[kSubports].size());

  SubportToForwardingState portStates;
  SubportToPartnerState portPartnerStates;
  for (auto const& port : json[kSubports]) {
    ports.emplace_hint(ports.cend(), Subport::fromFollyDynamic(port));
  }

  for (auto const& portAndState : json[kForwardingStates]) {
    auto id = static_cast<PortID>(portAndState[kPortID].asInt());
    Forwarding forwarding;
    if (portAndState[kForwarding] == "enabled") {
      forwarding = Forwarding::ENABLED;
    } else {
      CHECK_EQ(portAndState[kForwarding], "disabled");
      forwarding = Forwarding::DISABLED;
    }
    portStates[id] = forwarding;
  }

  for (auto const& portAndPartner : json[kPartnerInfos]) {
    auto id = static_cast<PortID>(portAndPartner[kPortID].asInt());
    auto partnerInfo =
        ParticipantInfo::fromFollyDynamic(portAndPartner[kPartnerInfo]);
    portPartnerStates[id] = partnerInfo;
  }

  return AggregatePortFields(
      AggregatePortID(json[kId].getInt()),
      json[kName].getString(),
      json[kDescription].getString(),
      json[kSystemPriority].getInt(),
      folly::MacAddress(json[kSystemID].getString()),
      json[kMinimumLinkCount].getInt(),
      std::move(ports),
      std::move(portStates),
      std::move(portPartnerStates));
}

AggregatePort::SubportsDifferenceType AggregatePort::subportsCount() const {
  auto subportsRange = sortedSubports();
  return std::distance(subportsRange.begin(), subportsRange.end());
}

uint32_t AggregatePort::forwardingSubportCount() const {
  uint32_t count = 0;

  AggregatePort::Forwarding fwdState;
  for (const auto& portAndState : subportAndFwdState()) {
    std::tie(std::ignore, fwdState) = portAndState;
    if (fwdState == Forwarding::ENABLED) {
      ++count;
    }
  }

  return count;
}

bool AggregatePort::isMemberPort(PortID port) const {
  for (const auto memberPort : getFields()->ports_) {
    if (memberPort.portID == port) {
      return true;
    }
  }

  return false;
}

AggregatePort* AggregatePort::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  AggregatePortMap* aggPorts = (*state)->getAggregatePorts()->modify(state);
  auto newAggPort = clone();
  auto* ptr = newAggPort.get();
  aggPorts->updateAggregatePort(std::move(newAggPort));
  return ptr;
}

// The physical port on which "packet" ingressed can fall into one of 3
// conditions.
// The physical ingress port
// case A: is CONFIGURED as a member of an AggregatePort AND is PROGRAMMED as a
//         member of that AggregatePort
// case B: is CONFIGURED as a member of an AggregatePort BUT is not PROGRAMMED
//         as a member of that AggregatePort
// case C: is not CONFIGURED as a member of any AggregatePort
bool AggregatePort::isIngressValid(
    const std::shared_ptr<SwitchState>& state,
    const std::unique_ptr<RxPacket>& packet) {
  auto physicalIngressPort = packet->getSrcPort();
  auto owningAggregatePort =
      state->getAggregatePorts()->getAggregatePortIf(physicalIngressPort);

  if (!owningAggregatePort) {
    // case C
    return true;
  }

  CHECK(owningAggregatePort);
  auto physicalIngressForwardingState =
      owningAggregatePort->getForwardingState(physicalIngressPort);

  bool isValid{false};
  switch (physicalIngressForwardingState) {
    case AggregatePort::Forwarding::ENABLED:
      // case A
      isValid = true;
      break;
    case AggregatePort::Forwarding::DISABLED:
      // case B
      isValid = false;
      break;
  }

  return isValid;
}

bool AggregatePort::isUp() const {
  return forwardingSubportCount() >= getMinimumLinkCount();
}

template class NodeBaseT<AggregatePort, AggregatePortFields>;

} // namespace facebook::fboss
