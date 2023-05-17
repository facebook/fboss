// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <boost/container/flat_set.hpp>

#include <fboss/agent/state/LabelForwardingEntry.h>
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Route.h"

namespace facebook::fboss {

using LabelForwardingRoute = NodeMapTraits<Label, LabelForwardingEntry>;

using LabelForwardingInformationBaseTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using LabelForwardingInformationBaseThriftType =
    std::map<int32_t, state::LabelForwardingEntryFields>;

class LabelForwardingInformationBase;
using LabelForwardingInformationBaseTraits = ThriftMapNodeTraits<
    LabelForwardingInformationBase,
    LabelForwardingInformationBaseTypeClass,
    LabelForwardingInformationBaseThriftType,
    LabelForwardingEntry>;

class LabelForwardingInformationBase
    : public ThriftMapNode<
          LabelForwardingInformationBase,
          LabelForwardingInformationBaseTraits> {
 public:
  using Base = ThriftMapNode<
      LabelForwardingInformationBase,
      LabelForwardingInformationBaseTraits>;
  using Traits = LabelForwardingInformationBaseTraits;
  using Base::modify;

  LabelForwardingInformationBase();

  virtual ~LabelForwardingInformationBase() override;

  LabelForwardingInformationBase* modify(std::shared_ptr<SwitchState>* state);

  LabelForwardingInformationBase* programLabel(
      std::shared_ptr<SwitchState>* state,
      Label label,
      ClientID client,
      AdminDistance distance,
      LabelNextHopSet nexthops);

  LabelForwardingInformationBase* unprogramLabel(
      std::shared_ptr<SwitchState>* state,
      Label label,
      ClientID client);

  LabelForwardingInformationBase* purgeEntriesForClient(
      std::shared_ptr<SwitchState>* state,
      ClientID client);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiLabelForwardingInformationBaseTypeClass =
    apache::thrift::type_class::map<
        apache::thrift::type_class::string,
        LabelForwardingInformationBaseTypeClass>;
using MultiLabelForwardingInformationBaseThriftType =
    std::map<std::string, LabelForwardingInformationBaseThriftType>;

class MultiLabelForwardingInformationBase;

using MultiLabelForwardingInformationBaseTraits =
    ThriftMultiSwitchMapNodeTraits<
        MultiLabelForwardingInformationBase,
        MultiLabelForwardingInformationBaseTypeClass,
        MultiLabelForwardingInformationBaseThriftType,
        LabelForwardingInformationBase>;

class HwSwitchMatcher;

class MultiLabelForwardingInformationBase
    : public ThriftMultiSwitchMapNode<
          MultiLabelForwardingInformationBase,
          MultiLabelForwardingInformationBaseTraits> {
 public:
  using Traits = MultiLabelForwardingInformationBaseTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiLabelForwardingInformationBase,
      MultiLabelForwardingInformationBaseTraits>;
  using BaseT::modify;

  MultiLabelForwardingInformationBase() {}
  virtual ~MultiLabelForwardingInformationBase() {}

  static bool isValidNextHopSet(const LabelNextHopSet& nexthops);

  // Used for resolving route when mpls rib is not enabled
  static void resolve(std::shared_ptr<LabelForwardingEntry> entry) {
    entry->setResolved(*(entry->getBestEntry().second));
  }

  MultiLabelForwardingInformationBase* modify(
      std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
