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

#include <boost/container/flat_set.hpp>
#include <folly/Range.h>

namespace facebook { namespace fboss {

class SwitchState;

struct AggregatePortFields {
  using Subports = boost::container::flat_set<PortID>;

  AggregatePortFields(
      AggregatePortID id,
      const std::string& name,
      const std::string& description,
      Subports&& ports);

  template<typename Fn>
  void forEachChild(Fn /* unused */) {}

  folly::dynamic toFollyDynamic() const;
  static AggregatePortFields fromFollyDynamic(const folly::dynamic& json);

  const AggregatePortID id_{0};
  std::string name_;
  std::string description_;
  Subports ports_;
};

/*
 * AggregatePort stores state for an IEEE 802.1AX link bundle.
 */
class AggregatePort : public NodeBaseT<AggregatePort, AggregatePortFields> {
 public:
  using SubportsDifferenceType = AggregatePortFields::Subports::difference_type;
  using SubportsConstRange =
      folly::Range<AggregatePortFields::Subports::const_iterator>;

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

  SubportsConstRange sortedSubports() const {
    return SubportsConstRange(
        getFields()->ports_.cbegin(), getFields()->ports_.cend());
  }

  template <typename ConstIter>
  void setSubports(folly::Range<ConstIter> subports) {
    writableFields()->ports_ = Subports(subports.begin(), subports.end());
  }

  SubportsDifferenceType subportsCount() const;

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;

  using Subports = AggregatePortFields::Subports;
};

}} // facebook::fboss
