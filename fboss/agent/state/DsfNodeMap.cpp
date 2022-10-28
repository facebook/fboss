// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

std::shared_ptr<DsfNode> DsfNodeMap::getDsfNodeIf(SwitchID switchId) const {
  return getNodeIf(static_cast<int64_t>(switchId));
}

void DsfNodeMap::addDsfNode(const std::shared_ptr<DsfNode>& dsfNode) {
  return addNode(dsfNode);
}

template class ThriftMapNode<DsfNodeMap, DsfNodeMapTraits>;

} // namespace facebook::fboss
