// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/if/gen-cpp2/mpls_constants.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/RouteNextHopsMulti.h"

namespace facebook::fboss {

using LabelNextHop = ResolvedNextHop;
using LabelNextHopEntry = RouteNextHopEntry;
using LabelNextHopSet = RouteNextHopSet;
using LabelNextHopsByClient = RouteNextHopsMulti;

struct LabelForwardingEntryFields {
  LabelForwardingEntryFields() {}
  LabelForwardingEntryFields(
      MplsLabel label,
      ClientID clientId,
      LabelNextHopEntry nexthop)
      : topLabel(label) {
    validateLabelNextHopEntry(nexthop);
    labelNextHopsByClient.update(clientId, std::move(nexthop));
  }

  MplsLabel topLabel;
  LabelNextHopEntry nexthop{LabelNextHopEntry::Action::DROP,
                            AdminDistance::MAX_ADMIN_DISTANCE};
  LabelNextHopsByClient labelNextHopsByClient;
  template <typename Fn>
  void forEachChild(Fn /* unused */) {}

  static void validateLabelNextHopEntry(const LabelNextHopEntry& nexthopentry);
};

class LabelForwardingEntry
    : public NodeBaseT<LabelForwardingEntry, LabelForwardingEntryFields> {
 public:
  using Label = MplsLabel;
  LabelForwardingEntry(
      Label topLabel,
      ClientID clientId,
      LabelNextHopEntry nexthop);

  Label getID() const {
    return getFields()->topLabel;
  }

  const LabelNextHopEntry& getLabelNextHop() const {
    return getFields()->nexthop;
  }

  const LabelNextHopsByClient& getLabelNextHopsByClient() const {
    return getFields()->labelNextHopsByClient;
  }

  bool isEmpty() const {
    return getFields()->labelNextHopsByClient.isEmpty();
  }

  const LabelNextHopEntry* FOLLY_NULLABLE
  getEntryForClient(ClientID clientId) const;

  void update(ClientID clientId, LabelNextHopEntry entry);

  void delEntryForClient(ClientID clientId);

  folly::dynamic toFollyDynamic() const override;

  static std::shared_ptr<LabelForwardingEntry> fromFollyDynamic(
      const folly::dynamic& json);

  bool operator==(const LabelForwardingEntry& rhs) const;

  LabelForwardingEntry* modify(std::shared_ptr<SwitchState>* state);

  std::string str() const;

 private:
  void updateLabelNextHop();
  // Inherit the constructors required for clone()
  using NodeBaseT<LabelForwardingEntry, LabelForwardingEntryFields>::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
