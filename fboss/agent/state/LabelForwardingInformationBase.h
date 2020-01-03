// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <boost/container/flat_set.hpp>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/state/NodeMap.h"

namespace facebook::fboss {

typedef NodeMapTraits<MplsLabel, LabelForwardingEntry> LabelForwardingRoute;

class LabelForwardingInformationBase
    : public NodeMapT<LabelForwardingInformationBase, LabelForwardingRoute> {
  using Label = MplsLabel;
  using BaseT = NodeMapT<LabelForwardingInformationBase, LabelForwardingRoute>;

 public:
  LabelForwardingInformationBase();

  ~LabelForwardingInformationBase();

  const std::shared_ptr<LabelForwardingEntry>& getLabelForwardingEntry(
      Label topLabel) const;

  std::shared_ptr<LabelForwardingEntry> getLabelForwardingEntryIf(
      Label topLabel) const;

  static std::shared_ptr<LabelForwardingInformationBase> fromFollyDynamic(
      const folly::dynamic& json);

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

  static bool isValidNextHopSet(const LabelNextHopSet& nexthops);

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
