#include "fboss/agent/state/LlrConfig.h"
#include "fboss/agent/state/NodeBase-defs.h"

namespace facebook::fboss {

template struct ThriftStructNode<LlrConfig, state::LlrFields>;

} // namespace facebook::fboss
