// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

template class ThriftMapNode<DsfNodeMap, DsfNodeMapTraits>;
} // namespace facebook::fboss
