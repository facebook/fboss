#include "fboss/agent/state/LlrConfigMap.h"

#include "fboss/agent/state/LlrConfig.h"
#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

MultiSwitchLlrConfigMap* MultiSwitchLlrConfigMap::modify(
    std::shared_ptr<SwitchState>* state) {
  return SwitchState::modify<switch_state_tags::llrCfgMaps>(state);
}

template struct ThriftMapNode<LlrConfigMap, LlrConfigMapTraits>;

} // namespace facebook::fboss
