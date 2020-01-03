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

namespace facebook::fboss {

ForwardingInformationBaseContainerFields::
    ForwardingInformationBaseContainerFields(RouterID vrf)
    : vrf(vrf) {
  fibV4 = std::make_shared<ForwardingInformationBaseV4>();
  fibV6 = std::make_shared<ForwardingInformationBaseV6>();
}

ForwardingInformationBaseContainer::ForwardingInformationBaseContainer(
    RouterID vrf)
    : NodeBaseT(vrf) {}

ForwardingInformationBaseContainer::~ForwardingInformationBaseContainer() {}

RouterID ForwardingInformationBaseContainer::getID() const {
  return getFields()->vrf;
}

const std::shared_ptr<ForwardingInformationBaseV4>&
ForwardingInformationBaseContainer::getFibV4() const {
  return getFields()->fibV4;
}
const std::shared_ptr<ForwardingInformationBaseV6>&
ForwardingInformationBaseContainer::getFibV6() const {
  return getFields()->fibV6;
}

std::shared_ptr<ForwardingInformationBaseContainer>
ForwardingInformationBaseContainer::fromFollyDynamic(
    const folly::dynamic& /* json */) {
  // TODO(samank)
  return nullptr;
}

folly::dynamic ForwardingInformationBaseContainer::toFollyDynamic() const {
  // TODO(samank)
  return folly::dynamic::object;
}

ForwardingInformationBaseContainer* ForwardingInformationBaseContainer::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  auto fibMap = (*state)->getFibs()->modify(state);
  auto newFibContainer = clone();

  auto* rtn = newFibContainer.get();
  fibMap->updateForwardingInformationBaseContainer(std::move(newFibContainer));

  return rtn;
}

template class NodeBaseT<
    ForwardingInformationBaseContainer,
    ForwardingInformationBaseContainerFields>;

} // namespace facebook::fboss
