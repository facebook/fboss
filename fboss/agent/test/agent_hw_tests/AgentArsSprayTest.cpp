// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/agent/AsicUtils.h"

#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/agent_hw_tests/AgentArsBase.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

using namespace facebook::fboss::utility;

namespace facebook::fboss {

const int kMaxLinks = 4;
const int kMinFlowletTableSize = 256;

const int kLoadWeight1 = 70;
const int kQueueWeight1 = 30;

using namespace ::testing;

class AgentArsSprayTest : public AgentArsBase {
 protected:
};

} // namespace facebook::fboss
