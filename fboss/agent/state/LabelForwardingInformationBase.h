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
  using Base::modify;

  LabelForwardingInformationBase();

  virtual ~LabelForwardingInformationBase() override;

  const std::shared_ptr<LabelForwardingEntry>& getLabelForwardingEntry(
      Label topLabel) const;

  std::shared_ptr<LabelForwardingEntry> getLabelForwardingEntryIf(
      Label topLabel) const;

  std::shared_ptr<LabelForwardingEntry> cloneLabelEntry(
      std::shared_ptr<LabelForwardingEntry> entry);

  LabelForwardingInformationBase* modify(std::shared_ptr<SwitchState>* state);

  LabelForwardingEntry* modifyLabelEntry(
      std::shared_ptr<SwitchState>* state,
      std::shared_ptr<LabelForwardingEntry> entry);

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

  static bool isValidNextHopSet(const LabelNextHopSet& nexthops);

  // Used for resolving route when mpls rib is not enabled
  static void resolve(std::shared_ptr<LabelForwardingEntry> entry) {
    entry->setResolved(*(entry->getBestEntry().second));
  }

  // For backward compatibility with old format
  static std::shared_ptr<LabelForwardingEntry> labelEntryFromFollyDynamic(
      folly::dynamic entry);

  static void noRibToRibEntryConvertor(
      std::shared_ptr<LabelForwardingEntry>& entry);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
  static std::shared_ptr<LabelForwardingEntry> fromFollyDynamicOldFormat(
      folly::dynamic entry);
};

} // namespace facebook::fboss
