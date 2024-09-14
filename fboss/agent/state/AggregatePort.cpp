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

namespace facebook::fboss {

AggregatePort::AggregatePort(
    AggregatePortID id,
    const std::string& name,
    const std::string& description,
    uint16_t systemPriority,
    folly::MacAddress systemID,
    uint8_t minimumLinkCount,
    Subports&& ports,
    const std::vector<int32_t>& interfaceIDs,
    LegacyAggregatePortFields::Forwarding fwd,
    ParticipantInfo pState) {
  set<switch_state_tags::id>(id);
  set<switch_state_tags::name>(name);
  set<switch_state_tags::description>(description);
  set<switch_state_tags::systemPriority>(systemPriority);
  set<switch_state_tags::systemID>(systemID.u64NBO());
  set<switch_state_tags::minimumLinkCount>(minimumLinkCount);
  std::vector<state::Subport> subPorts;
  for (const auto& subport : ports) {
    subPorts.push_back(subport.toThrift());
  }
  set<switch_state_tags::ports>(std::move(subPorts));
  std::map<int32_t, bool> portToFwdState{};
  for (const auto& subport : ports) {
    portToFwdState.emplace(subport.portID, fwd == Forwarding::ENABLED);
  }
  set<switch_state_tags::portToFwdState>(std::move(portToFwdState));
  std::map<int32_t, state::ParticipantInfo> portToPartnerState{};
  for (const auto& subport : ports) {
    portToPartnerState.emplace(subport.portID, pState.toThrift());
  }
  set<switch_state_tags::portToPartnerState>(std::move(portToPartnerState));
  set<switch_state_tags::interfaceIDs>(interfaceIDs);
}

AggregatePort::AggregatePort(
    AggregatePortID id,
    const std::string& name,
    const std::string& description,
    uint16_t systemPriority,
    folly::MacAddress systemID,
    uint8_t minLinkCount,
    Subports&& ports,
    SubportToForwardingState&& portStates,
    SubportToPartnerState&& portPartnerStates) {
  set<switch_state_tags::id>(id);
  set<switch_state_tags::name>(name);
  set<switch_state_tags::description>(description);
  set<switch_state_tags::systemPriority>(systemPriority);
  set<switch_state_tags::systemID>(systemID.u64NBO());
  set<switch_state_tags::minimumLinkCount>(minLinkCount);
  std::vector<state::Subport> subPorts;
  for (const auto& subport : ports) {
    subPorts.push_back(subport.toThrift());
  }
  std::map<int32_t, bool> portToFwdState{};
  for (const auto& [port, fwdState] : portStates) {
    portToFwdState.emplace(port, fwdState == Forwarding::ENABLED);
  }
  set<switch_state_tags::portToFwdState>(std::move(portToFwdState));
  std::map<int32_t, state::ParticipantInfo> portToPartnerState{};
  for (const auto& [port, partnerState] : portPartnerStates) {
    portToPartnerState.emplace(port, partnerState.toThrift());
  }
  set<switch_state_tags::portToPartnerState>(std::move(portToPartnerState));
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
  for (const auto& memberPort :
       std::as_const(*cref<switch_state_tags::ports>())) {
    if (PortID(memberPort->cref<switch_state_tags::id>()->cref()) == port) {
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

  auto* aggPorts = (*state)->getAggregatePorts()->modify(state);
  auto [_, scope] = aggPorts->getNodeAndScope(getID());
  auto newAggPort = clone();
  auto* ptr = newAggPort.get();
  aggPorts->updateNode(newAggPort, scope);
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
    const std::unique_ptr<RxPacket>& packet,
    const bool needAggPortUp) {
  auto physicalIngressPort = packet->getSrcPort();
  auto owningAggregatePort =
      state->getAggregatePorts()->getAggregatePortForPort(physicalIngressPort);

  if (!owningAggregatePort) {
    // case C
    return true;
  }

  CHECK(owningAggregatePort);
  if (needAggPortUp) {
    return owningAggregatePort->isUp();
  }
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

template class ThriftStructNode<AggregatePort, state::AggregatePortFields>;

} // namespace facebook::fboss
