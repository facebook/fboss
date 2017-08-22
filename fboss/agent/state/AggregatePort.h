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

#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"
#include "fboss/agent/FbossError.h"

#include <boost/container/flat_set.hpp>
#include <folly/Range.h>

namespace facebook { namespace fboss {

class SwitchState;

struct AggregatePortFields {
  using Subports = boost::container::flat_set<PortID>;

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

  AggregatePortFields(
      AggregatePortID id,
      const std::string& name,
      const std::string& description,
      Subports&& ports,
      Forwarding fwd = Forwarding::ENABLED);

  template<typename Fn>
  void forEachChild(Fn /* unused */) {}

  folly::dynamic toFollyDynamic() const;
  static AggregatePortFields fromFollyDynamic(const folly::dynamic& json);

  const AggregatePortID id_{0};
  std::string name_;
  std::string description_;
  Subports ports_;
  SubportToForwardingState portToFwdState_;
};

/*
 * AggregatePort stores state for an IEEE 802.1AX link bundle.
 */
class AggregatePort : public NodeBaseT<AggregatePort, AggregatePortFields> {
 public:
  using SubportsDifferenceType = AggregatePortFields::Subports::difference_type;
  using SubportsConstRange =
      folly::Range<AggregatePortFields::Subports::const_iterator>;
  using Forwarding = AggregatePortFields::Forwarding;
  using SubportAndForwardingStateConstRange = folly::Range<
      AggregatePortFields::SubportToForwardingState::const_iterator>;
  using SubportAndForwardingStateValueType =
      AggregatePortFields::SubportToForwardingState::value_type;

  template<typename Iterator>
  static std::shared_ptr<AggregatePort> fromSubportRange(
      AggregatePortID id,
      const std::string& name,
      const std::string& description,
      folly::Range<Iterator> subports) {
    return std::make_shared<AggregatePort>(
        id, name, description, Subports(subports.begin(), subports.end()));
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

  AggregatePort::Forwarding getForwardingState(PortID port) {
    auto it = getFields()->portToFwdState_.find(port);
    if (it == getFields()->portToFwdState_.cend()) {
      throw FbossError("No forwarding state found for port ", port);
    }

    return it->second;
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

  SubportAndForwardingStateConstRange subportAndFwdState() const {
    return SubportAndForwardingStateConstRange(
        getFields()->portToFwdState_.cbegin(),
        getFields()->portToFwdState_.cend());
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;

  using Subports = AggregatePortFields::Subports;
};

}} // facebook::fboss
