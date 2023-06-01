/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/SflowCollectorMap.h"

#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

template class ThriftMapNode<SflowCollectorMap, SflowCollectorMapTraits>;

MultiSwitchSflowCollectorMap* MultiSwitchSflowCollectorMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::sflowCollectorMaps>(state);
}
} // namespace facebook::fboss
