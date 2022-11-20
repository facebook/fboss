#include "fboss/agent/state/UdfGroupMap.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/UdfGroup.h"

namespace facebook::fboss {

std::shared_ptr<UdfGroup> UdfGroupMap::getUdfGroupIf(
    const std::string& name) const {
  return getNodeIf(name);
}

void UdfGroupMap::addUdfGroup(const std::shared_ptr<UdfGroup>& udfGroup) {
  return addNode(udfGroup);
}

template class ThriftMapNode<UdfGroupMap, UdfGroupMapTraits>;

} // namespace facebook::fboss
