/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

namespace {
constexpr auto kFibV4{"fibV4"};
constexpr auto kFibV6{"fibV6"};
constexpr auto kVrf{"vrf"};
} // namespace

namespace facebook::fboss {

ForwardingInformationBaseContainer::ForwardingInformationBaseContainer(
    RouterID vrf) {
  set<switch_state_tags::vrf>(vrf);
}

ForwardingInformationBaseContainer::~ForwardingInformationBaseContainer() {}

RouterID ForwardingInformationBaseContainer::getID() const {
  return RouterID(get<switch_state_tags::vrf>()->cref());
}

const std::shared_ptr<ForwardingInformationBaseV4>&
ForwardingInformationBaseContainer::getFibV4() const {
  return this->cref<switch_state_tags::fibV4>();
}
const std::shared_ptr<ForwardingInformationBaseV6>&
ForwardingInformationBaseContainer::getFibV6() const {
  return this->cref<switch_state_tags::fibV6>();
}

ForwardingInformationBaseContainer* ForwardingInformationBaseContainer::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  auto fibMap = (*state)->getFibs()->modify(state);
  auto [node, scope] = fibMap->getNodeAndScope(getID());
  DCHECK_EQ(node.get(), this);
  auto newFibContainer = clone();

  auto* rtn = newFibContainer.get();
  fibMap->updateForwardingInformationBaseContainer(
      std::move(newFibContainer), scope);

  return rtn;
}

template class ThriftStructNode<
    ForwardingInformationBaseContainer,
    state::FibContainerFields>;

} // namespace facebook::fboss
