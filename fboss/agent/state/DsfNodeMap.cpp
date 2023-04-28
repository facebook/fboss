// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/DsfNode.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

void MultiDsfNodeMap::addNode(
    std::shared_ptr<DsfNode> node,
    const HwSwitchMatcher& matcher) {
  const auto& key = matcher.matcherString();
  auto mitr = find(key);
  if (mitr == end()) {
    mitr = insert(key, std::make_shared<DsfNodeMap>()).first;
  }
  auto& map = mitr->second;
  map->addNode(std::move(node));
}

size_t MultiDsfNodeMap::numNodes() const {
  size_t cnt = 0;
  for (auto mnitr = cbegin(); mnitr != cend(); ++mnitr) {
    cnt += mnitr->second->size();
  }
  return cnt;
}

void MultiDsfNodeMap::updateNode(
    std::shared_ptr<DsfNode> node,
    const HwSwitchMatcher& matcher) {
  const auto& key = matcher.matcherString();
  auto mitr = find(key);
  if (mitr == end()) {
    throw FbossError("No nodes found for switchIds: ", key);
  }
  auto& nodeMap = mitr->second;
  nodeMap->updateNode(std::move(node));
}

void MultiDsfNodeMap::removeNode(SwitchID switchId) {
  for (auto mitr = begin(); mitr != end(); ++mitr) {
    if (mitr->second->remove(switchId)) {
      return;
    }
  }
  throw FbossError("Node not found: ", switchId);
}

std::shared_ptr<DsfNode> MultiDsfNodeMap::getNodeIf(SwitchID switchId) const {
  for (auto mnitr = cbegin(); mnitr != cend(); ++mnitr) {
    auto node = mnitr->second->getNodeIf(switchId);
    if (node) {
      return node;
    }
  }
  return nullptr;
}
template class ThriftMapNode<DsfNodeMap, DsfNodeMapTraits>;
} // namespace facebook::fboss
