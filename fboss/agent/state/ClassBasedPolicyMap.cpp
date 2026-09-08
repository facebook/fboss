/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/ClassBasedPolicyMap.h"

#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

ClassBasedPolicyMap::ClassBasedPolicyMap() = default;

ClassBasedPolicyMap::~ClassBasedPolicyMap() = default;

MultiSwitchClassBasedPolicyMap* MultiSwitchClassBasedPolicyMap::modify(
    std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  SwitchState::modify(state);
  auto newMultiSwitchMap = clone();
  for (auto mnitr = cbegin(); mnitr != cend(); ++mnitr) {
    (*newMultiSwitchMap)[mnitr->first] = mnitr->second->clone();
  }
  auto* ptr = newMultiSwitchMap.get();
  (*state)->resetClassBasedPolicies(newMultiSwitchMap);
  return ptr;
}

template struct ThriftMapNode<ClassBasedPolicyMap, ClassBasedPolicyMapTraits>;

} // namespace facebook::fboss
