// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

MultiSwitchDsfNodeMap* MultiSwitchDsfNodeMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::dsfNodesMap>(state);
}

template class ThriftMapNode<DsfNodeMap, DsfNodeMapTraits>;
} // namespace facebook::fboss
