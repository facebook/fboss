// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include <fboss/agent/state/LabelForwardingEntry.h>
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

// Enable label route handling in Routing Information Base
// which allows recursive resolution of mpls routes
DEFINE_bool(mpls_rib, true, "Enable mpls rib");

namespace facebook::fboss {

namespace {
auto constexpr kIncomingLabel = "topLabel";
auto constexpr kLabelNextHop = "labelNextHop";
auto constexpr kLabelNextHopsByClient = "labelNextHopMulti";
} // namespace

LabelForwardingInformationBase::LabelForwardingInformationBase() {}

LabelForwardingInformationBase::~LabelForwardingInformationBase() {}

LabelForwardingInformationBase* LabelForwardingInformationBase::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  auto newFib = this->clone();
  auto* ptr = newFib.get();
  (*state)->resetLabelForwardingInformationBase(std::move(newFib));
  return ptr;
}

bool MultiLabelForwardingInformationBase::isValidNextHopSet(
    const LabelNextHopSet& nexthops) {
  for (const auto& nexthop : nexthops) {
    if (!nexthop.labelForwardingAction()) {
      XLOG(ERR) << "missing label forwarding action in " << nexthop.str();
      return false;
    }
    if (nexthop.isPopAndLookup() && nexthops.size() > 1) {
      /* pop and lookup forwarding action does not have and need interface id
      as well as next hop address, accordingly it is always valid. however
      there must be only one next hop with pop and lookup */
      XLOG(ERR) << "nexthop set with pop and lookup exceed size 1";
      return false;
    } else if (!nexthop.isPopAndLookup() && !nexthop.isResolved()) {
      XLOG(ERR) << "next hop is not resolved, " << nexthop.str();
      return false;
    }
  }
  return true;
}

MultiLabelForwardingInformationBase*
MultiLabelForwardingInformationBase::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::labelFibMap>(state);
}

template class ThriftMapNode<
    LabelForwardingInformationBase,
    LabelForwardingInformationBaseTraits>;

} // namespace facebook::fboss
