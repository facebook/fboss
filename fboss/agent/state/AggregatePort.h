/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_set.hpp>
#include <folly/MacAddress.h>
#include <folly/Range.h>

namespace facebook::fboss {

class RxPacket;
class SwitchState;

struct LegacyAggregatePortFields {
  /* The SDK exposes much finer controls over the egress state of trunk member
   * ports, both as compared to what we expose in SwSwitch and as compared to
   * the ingress trunk member port control. I don't see a need for these more
   * granular states at this time.
   */
  enum class Forwarding { ENABLED, DISABLED };

  /* Instead of introducing a new data structure, we could have chosen to
   * maintain the Forwarding state in the Subports structure by modifying
   * Subports to hold pairs of type (PortID,Forwarding). This layout is less
   * wasteful so I had initially chosen it. But becuse I wanted to avoid
   * modifying callsites of AggregatePort::setSubports() and
   * AggregatePort::getSubports(), I had modified these methods (and ctors) to
   * use iterator transformations to hide the underlying layout of pairs.
   * The fallout of that is here: P57596006. This turned out to be more painful
   * than I had initially anticipated. So to avoid modifying AggregatePort
   * callsites to take into Forwarding state (even when Forwarding state is
   * strictly unrelated to the context of the callsite, like in
   * ApplyThriftConfig), I split out Forwarding state into its own data
   * structure. Because there's never a need to set the Forwarding state on
   * member ports during initialization, having two separate data structures
   * (ie. Subports and SubportToForwardingState) allows for keeping the
   * callsites intact). As an added bonus, separate data structures feels more
   * natural in BcmTrunk::program(...) when taking diffs between AggregatePort
   * objects.
   */
  using SubportToForwardingState =
      boost::container::flat_map<PortID, Forwarding>;

  using SubportToPartnerState =
      boost::container::flat_map<PortID, ParticipantInfo>;

  struct Subport {
    Subport() = default;
    Subport(
        PortID id,
        uint16_t pri,
        cfg::LacpPortRate r,
        cfg::LacpPortActivity a,
        uint16_t mul)
        : portID(id),
          priority(pri),
          rate(r),
          activity(a),
          holdTimerMulitiplier(mul) {}

    // Needed for std::equal
    bool operator==(const Subport& rhs) const {
      return portID == rhs.portID && priority == rhs.priority &&
          rate == rhs.rate && activity == rhs.activity &&
          holdTimerMulitiplier == rhs.holdTimerMulitiplier;
    }
    bool operator!=(const Subport& rhs) const {
      return !(*this == rhs);
    }

    // Needed for boost::container::flat_set
    bool operator<(const Subport& rhs) const {
      return portID < rhs.portID;
    }

    state::Subport toThrift() const {
      state::Subport subport;
      subport.id() = portID;
      subport.priority() = priority;
      subport.lacpPortRate() = rate;
      subport.lacpPortActivity() = activity;
      subport.holdTimerMultiplier() = holdTimerMulitiplier;
      return subport;
    }

    static Subport fromThrift(state::Subport subport) {
      return Subport(
          PortID(*subport.id()),
          *subport.priority(),
          *subport.lacpPortRate(),
          *subport.lacpPortActivity(),
          *subport.holdTimerMultiplier());
    }

    PortID portID{0};
    uint16_t priority{0};
    cfg::LacpPortRate rate{cfg::LacpPortRate::SLOW};
    cfg::LacpPortActivity activity{cfg::LacpPortActivity::PASSIVE};
    uint16_t holdTimerMulitiplier{
        cfg::switch_config_constants::DEFAULT_LACP_HOLD_TIMER_MULTIPLIER()};
  };
  using Subports = boost::container::flat_set<Subport>;
};

/*
 * AggregatePort stores state for an IEEE 802.1AX link bundle.
 */
class AggregatePort
    : public ThriftStructNode<AggregatePort, state::AggregatePortFields> {
 public:
  using Subport = LegacyAggregatePortFields::Subport;
  using SubportsDifferenceType =
      LegacyAggregatePortFields::Subports::difference_type;
  using SubportsConstRange =
      folly::Range<LegacyAggregatePortFields::Subports::const_iterator>;
  using Forwarding = LegacyAggregatePortFields::Forwarding;
  using PartnerState = ParticipantInfo;
  using SubportAndForwardingStateConstRange = folly::Range<
      LegacyAggregatePortFields::SubportToForwardingState::const_iterator>;
  using SubportAndForwardingStateValueType =
      LegacyAggregatePortFields::SubportToForwardingState::value_type;
  using ThriftType = state::AggregatePortFields;
  using Base = ThriftStructNode<AggregatePort, state::AggregatePortFields>;
  using Base::modify;
  using Subports = boost::container::flat_set<Subport>;

  using SubportToForwardingState =
      boost::container::flat_map<PortID, Forwarding>;

  using SubportToPartnerState =
      boost::container::flat_map<PortID, ParticipantInfo>;

  AggregatePort(
      AggregatePortID id,
      const std::string& name,
      const std::string& description,
      uint16_t systemPriority,
      folly::MacAddress systemID,
      uint8_t minimumLinkCount,
      Subports&& ports,
      const std::vector<int32_t>& interfaceIDs,
      LegacyAggregatePortFields::Forwarding fwd = Forwarding::DISABLED,
      ParticipantInfo pState = ParticipantInfo::defaultParticipantInfo());

  AggregatePort(
      AggregatePortID id,
      const std::string& name,
      const std::string& description,
      uint16_t systemPriority,
      folly::MacAddress systemID,
      uint8_t minLinkCount,
      Subports&& ports,
      SubportToForwardingState&& portStates,
      SubportToPartnerState&& portPartnerStates);

  template <typename Iterator>
  static std::shared_ptr<AggregatePort> fromSubportRange(
      AggregatePortID id,
      const std::string& name,
      const std::string& description,
      uint16_t systemPriority,
      folly::MacAddress systemID,
      uint8_t minLinkCount,
      folly::Range<Iterator> subports,
      const std::vector<int32_t>& interfaceIDs) {
    return std::make_shared<AggregatePort>(
        id,
        name,
        description,
        systemPriority,
        systemID,
        minLinkCount,
        Subports(subports.begin(), subports.end()),
        interfaceIDs,
        Forwarding::DISABLED,
        ParticipantInfo::defaultParticipantInfo());
  }

  AggregatePortID getID() const {
    return AggregatePortID(cref<switch_state_tags::id>()->cref());
  }

  const std::string& getName() const {
    return cref<switch_state_tags::name>()->cref();
  }

  void setName(const std::string& name) {
    set<switch_state_tags::name>(name);
  }

  const std::string& getDescription() const {
    return cref<switch_state_tags::description>()->cref();
  }

  void setDescription(const std::string& desc) {
    set<switch_state_tags::description>(desc);
  }

  uint16_t getSystemPriority() const {
    return cref<switch_state_tags::systemPriority>()->cref();
  }

  void setSystemPriority(uint16_t systemPriority) {
    set<switch_state_tags::systemPriority>(systemPriority);
  }

  folly::MacAddress getSystemID() const {
    return folly::MacAddress::fromNBO(
        cref<switch_state_tags::systemID>()->cref());
  }

  void setSystemID(folly::MacAddress systemID) {
    set<switch_state_tags::systemID>(systemID.u64NBO());
  }

  uint8_t getMinimumLinkCount() const {
    return cref<switch_state_tags::minimumLinkCount>()->cref();
  }

  void setMinimumLinkCount(uint8_t minLinkCount) {
    set<switch_state_tags::minimumLinkCount>(minLinkCount);
  }

  AggregatePort::Forwarding getForwardingState(PortID port) {
    const auto& portToFwdState = cref<switch_state_tags::portToFwdState>();
    auto it = std::as_const(*portToFwdState).find(port);
    if (it == portToFwdState->cend()) {
      throw FbossError("No forwarding state found for port ", port);
    }
    return (it->second->cref() == true) ? Forwarding::ENABLED
                                        : Forwarding::DISABLED;
  }

  void setForwardingState(PortID port, AggregatePort::Forwarding fwd) {
    auto portToFwdState = get<switch_state_tags::portToFwdState>()->clone();
    auto it = portToFwdState->find(port);
    if (it == portToFwdState->cend()) {
      throw FbossError("No forwarding state found for port ", port);
    }
    it->second->ref() = (fwd == Forwarding::ENABLED);
    ref<switch_state_tags::portToFwdState>() = portToFwdState;
  }

  // THRIFT_COPY
  AggregatePort::PartnerState getPartnerState(PortID port) const {
    const auto& portToPartnerState =
        cref<switch_state_tags::portToPartnerState>();
    auto it = std::as_const(*portToPartnerState).find(port);
    if (it == portToPartnerState->cend()) {
      throw FbossError("No partner state found for port ", port);
    }

    return AggregatePort::PartnerState::fromThrift(it->second->toThrift());
  }

  // THRIFT_COPY
  void setPartnerState(PortID port, const AggregatePort::PartnerState& state) {
    auto portToPartnerState =
        get<switch_state_tags::portToPartnerState>()->clone();
    auto it = portToPartnerState->find(port);
    if (it == portToPartnerState->end()) {
      throw FbossError("No partner state found for port ", port);
    }
    auto partnerState = portToPartnerState->cref(port)->clone();
    partnerState->fromThrift(state.toThrift());
    portToPartnerState->ref(port) = partnerState;
    ref<switch_state_tags::portToPartnerState>() = portToPartnerState;
  }

  // THRIFT_COPY
  std::vector<Subport> sortedSubports() const {
    std::vector<Subport> subports;
    for (const auto& subport :
         std::as_const(*cref<switch_state_tags::ports>())) {
      subports.push_back(Subport::fromThrift(subport->toThrift()));
    }
    return subports;
  }

  // THRIFT_COPY
  template <typename ConstIter>
  void setSubports(folly::Range<ConstIter> subports) {
    auto subportMap = Subports(subports.begin(), subports.end());
    std::vector<state::Subport> subportsThrift{};
    for (const auto& subport : subportMap) {
      subportsThrift.push_back(subport.toThrift());
    }
    set<switch_state_tags::ports>(std::move(subportsThrift));
  }

  uint32_t subportsCount() const {
    return cref<switch_state_tags::ports>()->size();
  }

  uint32_t forwardingSubportCount() const;

  LegacyAggregatePortFields::SubportToForwardingState subportAndFwdState()
      const {
    LegacyAggregatePortFields::SubportToForwardingState portToFwdState;

    for (const auto& [key, val] :
         std::as_const(*cref<switch_state_tags::portToFwdState>())) {
      portToFwdState[PortID(key)] =
          (val->cref() == true) ? Forwarding::ENABLED : Forwarding::DISABLED;
    }
    return portToFwdState;
  }

  bool isMemberPort(PortID port) const;

  auto getInterfaceIDs() const {
    return safe_cref<switch_state_tags::interfaceIDs>();
  }

  void setInterfaceIDs(const std::vector<int32_t>& interfaceIDs) {
    set<switch_state_tags::interfaceIDs>(interfaceIDs);
  }

  AggregatePort* modify(std::shared_ptr<SwitchState>* state);

  static bool isIngressValid(
      const std::shared_ptr<SwitchState>& state,
      const std::unique_ptr<RxPacket>& packet,
      const bool needAggPortUp = false);

  bool isUp() const;

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
