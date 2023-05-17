/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/QosPolicyMap.h"

#include "fboss/agent/state/NodeMap-defs.h"

#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

QosPolicyMap::QosPolicyMap() {}

QosPolicyMap::~QosPolicyMap() {}

QosPolicyMap* QosPolicyMap::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newQosPolicies = clone();
  auto* ptr = newQosPolicies.get();
  (*state)->resetQosPolicies(std::move(newQosPolicies));
  return ptr;
}

MultiSwitchQosPolicyMap* MultiSwitchQosPolicyMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::qosPolicyMaps>(state);
}

template class ThriftMapNode<QosPolicyMap, QosPolicyMapTraits>;

} // namespace facebook::fboss
