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
} // namespace

namespace facebook::fboss {

folly::dynamic AggregatePortFields::Subport::toFollyDynamic(
    const AggregatePortFields::SubportToForwardingState& portStates) const {
  folly::dynamic subport = folly::dynamic::object;
  subport[kPortID] = static_cast<uint16_t>(portID);
  subport[kPriority] = static_cast<uint16_t>(priority);
  subport[kRate] = rate == cfg::LacpPortRate::FAST ? "fast" : "slow";
  subport[kActivity] =
      activity == cfg::LacpPortActivity::ACTIVE ? "active" : "passive";
  subport[kForwarding] =
      portStates.at(portID) == Forwarding::ENABLED ? "enabled" : "disabled";
  return subport;
}

AggregatePortFields::Subport AggregatePortFields::Subport::fromFollyDynamic(
    const folly::dynamic& json,
    AggregatePortFields::SubportToForwardingState& portStates) {
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

  Forwarding forwarding;
  if (json[kForwarding] == "enabled") {
    forwarding = Forwarding::ENABLED;
  } else {
    CHECK_EQ(json[kForwarding], "disabled");
    forwarding = Forwarding::DISABLED;
  }

  // TODO(samank): check widths match up
  auto id = static_cast<PortID>(json[kPortID].asInt());
  auto priority = static_cast<uint16_t>(json[kPriority].asInt());

  portStates[id] = forwarding;
  return Subport(id, priority, rate, activity);
}

AggregatePortFields::AggregatePortFields(
    AggregatePortID id,
    const std::string& name,
    const std::string& description,
    uint16_t systemPriority,
    folly::MacAddress systemID,
    uint8_t minimumLinkCount,
    Subports&& ports,
    SubportToForwardingState&& portStates)
    : id_(id),
      name_(name),
      description_(description),
      systemPriority_(systemPriority),
      systemID_(systemID),
      minimumLinkCount_(minimumLinkCount),
      ports_(std::move(ports)),
      portToFwdState_(std::move(portStates)) {}

AggregatePortFields::AggregatePortFields(
    AggregatePortID id,
    const std::string& name,
    const std::string& description,
    uint16_t systemPriority,
    folly::MacAddress systemID,
    uint8_t minimumLinkCount,
    Subports&& ports,
    AggregatePortFields::Forwarding fwd)
    : id_(id),
      name_(name),
      description_(description),
      systemPriority_(systemPriority),
      systemID_(systemID),
      minimumLinkCount_(minimumLinkCount),
      ports_(std::move(ports)),
      portToFwdState_() {
  for (const auto& subport : ports_) {
    auto hint = portToFwdState_.end();
    portToFwdState_.emplace_hint(hint, subport.portID, fwd);
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
    subports.push_back(subport.toFollyDynamic(portToFwdState_));
  }
  aggPortFields[kSubports] = std::move(subports);
  aggPortFields[kSystemID] = systemID_.toString();
  aggPortFields[kSystemPriority] = static_cast<uint16_t>(systemPriority_);
  return aggPortFields;
}

AggregatePortFields AggregatePortFields::fromFollyDynamic(
    const folly::dynamic& json) {
  Subports ports;

  ports.reserve(json[kSubports].size());

  SubportToForwardingState portStates;
  for (auto const& port : json[kSubports]) {
    ports.emplace_hint(
        ports.cend(), Subport::fromFollyDynamic(port, portStates));
  }

  return AggregatePortFields(
      AggregatePortID(json[kId].getInt()),
      json[kName].getString(),
      json[kDescription].getString(),
      json[kSystemPriority].getInt(),
      folly::MacAddress(json[kSystemID].getString()),
      json[kMinimumLinkCount].getInt(),
      std::move(ports),
      std::move(portStates));
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
