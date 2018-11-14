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

namespace facebook {
namespace fboss {

ForwardingInformationBaseContainerFields::
    ForwardingInformationBaseContainerFields(RouterID vrf)
    : vrf(vrf) {}

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

template class NodeBaseT<
    ForwardingInformationBaseContainer,
    ForwardingInformationBaseContainerFields>;

} // namespace fboss
} // namespace facebook
