#include "fboss/agent/state/UdfPacketMatcherMap.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/UdfPacketMatcher.h"

namespace facebook::fboss {

std::shared_ptr<UdfPacketMatcher> UdfPacketMatcherMap::getUdfPacketMatcherIf(
    const std::string& name) const {
  return getNodeIf(name);
}

void UdfPacketMatcherMap::addUdfPacketMatcher(
    const std::shared_ptr<UdfPacketMatcher>& udfPacketMatcher) {
  return addNode(udfPacketMatcher);
}

template class ThriftMapNode<UdfPacketMatcherMap, UdfPacketMatcherMapTraits>;

} // namespace facebook::fboss
