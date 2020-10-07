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
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_set.hpp>
#include <folly/MacAddress.h>
#include <folly/Range.h>

namespace facebook::fboss {

class RxPacket;
class SwitchState;

struct AggregatePortFields {
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
    Subport() {}
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

    folly::dynamic toFollyDynamic() const;
    static Subport fromFollyDynamic(const folly::dynamic& json);

    PortID portID{0};
    uint16_t priority{0};
    cfg::LacpPortRate rate{cfg::LacpPortRate::SLOW};
    cfg::LacpPortActivity activity{cfg::LacpPortActivity::PASSIVE};
    uint16_t holdTimerMulitiplier{
        cfg::switch_config_constants::DEFAULT_LACP_HOLD_TIMER_MULTIPLIER()};
  };
  using Subports = boost::container::flat_set<Subport>;

  AggregatePortFields(
      AggregatePortID id,
      const std::string& name,
      const std::string& description,
      uint16_t systemPriority,
      folly::MacAddress systemID,
      uint8_t minLinkCount,
      Subports&& ports,
      SubportToForwardingState&& portStates,
      SubportToPartnerState&& portPartnerStates);

  AggregatePortFields(
      AggregatePortID id,
      const std::string& name,
      const std::string& description,
      uint16_t systemPriority,
      folly::MacAddress systemID,
      uint8_t minLinkCount,
      Subports&& ports,
      Forwarding fwd = Forwarding::DISABLED,
      ParticipantInfo pState = ParticipantInfo::defaultParticipantInfo());

  template <typename Fn>
  void forEachChild(Fn /* unused */) {}

  folly::dynamic toFollyDynamic() const;
  static AggregatePortFields fromFollyDynamic(const folly::dynamic& json);

  const AggregatePortID id_{0};
  std::string name_;
  std::string description_;
  // systemPriortity_ and systemID_ are LACP parameters associated with an
  // entire system; they are constant across AggregatePorts. Maintaining a copy
  // of these parameters in each AggregatePort node is a convenient way of
  // signalling to LinkAggregationManager that they've been updated via a config
  // change.
  uint16_t systemPriority_;
  folly::MacAddress systemID_;
  uint8_t minimumLinkCount_;
  Subports ports_;
  SubportToForwardingState portToFwdState_;
  SubportToPartnerState portToPartnerState_;

 private:
  folly::dynamic portAndFwdStateToFollyDynamic(
      const std::pair<PortID, Forwarding>& fwdState) const;

  folly::dynamic portAndPartnerStateToFollyDynamic(
      const std::pair<PortID, ParticipantInfo>& partnerState) const;
};

/*
 * AggregatePort stores state for an IEEE 802.1AX link bundle.
 */
class AggregatePort : public NodeBaseT<AggregatePort, AggregatePortFields> {
 public:
  using Subport = AggregatePortFields::Subport;
  using SubportsDifferenceType = AggregatePortFields::Subports::difference_type;
  using SubportsConstRange =
      folly::Range<AggregatePortFields::Subports::const_iterator>;
  using Forwarding = AggregatePortFields::Forwarding;
  using PartnerState = ParticipantInfo;
  using SubportAndForwardingStateConstRange = folly::Range<
      AggregatePortFields::SubportToForwardingState::const_iterator>;
  using SubportAndForwardingStateValueType =
      AggregatePortFields::SubportToForwardingState::value_type;

  template <typename Iterator>
  static std::shared_ptr<AggregatePort> fromSubportRange(
      AggregatePortID id,
      const std::string& name,
      const std::string& description,
      uint16_t systemPriority,
      folly::MacAddress systemID,
      uint8_t minLinkCount,
      folly::Range<Iterator> subports) {
    return std::make_shared<AggregatePort>(
        id,
        name,
        description,
        systemPriority,
        systemID,
        minLinkCount,
        Subports(subports.begin(), subports.end()));
  }

  static std::shared_ptr<AggregatePort> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = AggregatePortFields::fromFollyDynamic(json);
    return std::make_shared<AggregatePort>(fields);
  }

  static std::shared_ptr<AggregatePort> fromJson(
      const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  AggregatePortID getID() const {
    return getFields()->id_;
  }

  const std::string& getName() const {
    return getFields()->name_;
  }

  void setName(const std::string& name) {
    writableFields()->name_ = name;
  }

  const std::string& getDescription() const {
    return getFields()->description_;
  }

  void setDescription(const std::string& desc) {
    writableFields()->description_ = desc;
  }

  uint16_t getSystemPriority() const {
    return getFields()->systemPriority_;
  }

  void setSystemPriority(uint16_t systemPriority) {
    writableFields()->systemPriority_ = systemPriority;
  }

  folly::MacAddress getSystemID() const {
    return getFields()->systemID_;
  }

  void setSystemID(folly::MacAddress systemID) {
    writableFields()->systemID_ = systemID;
  }

  uint8_t getMinimumLinkCount() const {
    return getFields()->minimumLinkCount_;
  }

  void setMinimumLinkCount(uint8_t minLinkCount) {
    writableFields()->minimumLinkCount_ = minLinkCount;
  }

  AggregatePort::Forwarding getForwardingState(PortID port) {
    auto it = getFields()->portToFwdState_.find(port);
    if (it == getFields()->portToFwdState_.cend()) {
      throw FbossError("No forwarding state found for port ", port);
    }

    return it->second;
  }

  void setForwardingState(PortID port, AggregatePort::Forwarding fwd) {
    // TODO(samank): need to handle case in which port doesn't exist?
    auto it = writableFields()->portToFwdState_.find(port);
    if (it == writableFields()->portToFwdState_.end()) {
      throw FbossError("No forwarding state found for port ", port);
    }

    it->second = fwd;
  }

  AggregatePort::PartnerState getPartnerState(PortID port) const {
    auto it = getFields()->portToPartnerState_.find(port);
    if (it == getFields()->portToPartnerState_.cend()) {
      throw FbossError("No partner state found for port ", port);
    }

    return it->second;
  }

  void setPartnerState(PortID port, const AggregatePort::PartnerState& state) {
    auto it = writableFields()->portToPartnerState_.find(port);
    if (it == writableFields()->portToPartnerState_.end()) {
      throw FbossError("No partner state found for port ", port);
    }

    it->second = state;
  }

  SubportsConstRange sortedSubports() const {
    return SubportsConstRange(
        getFields()->ports_.cbegin(), getFields()->ports_.cend());
  }

  template <typename ConstIter>
  void setSubports(folly::Range<ConstIter> subports) {
    writableFields()->ports_ = Subports(subports.begin(), subports.end());
  }

  SubportsDifferenceType subportsCount() const;

  uint32_t forwardingSubportCount() const;

  SubportAndForwardingStateConstRange subportAndFwdState() const {
    return SubportAndForwardingStateConstRange(
        getFields()->portToFwdState_.cbegin(),
        getFields()->portToFwdState_.cend());
  }

  bool isMemberPort(PortID port) const;

  AggregatePort* modify(std::shared_ptr<SwitchState>* state);

  static bool isIngressValid(
      const std::shared_ptr<SwitchState>& state,
      const std::unique_ptr<RxPacket>& packet);

  bool isUp() const;

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;

  using Subports = AggregatePortFields::Subports;
};

} // namespace facebook::fboss
