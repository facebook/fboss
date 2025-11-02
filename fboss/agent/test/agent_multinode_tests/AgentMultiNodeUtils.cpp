// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeUtils.h"

#include "fboss/agent/test/thrift_client_utils/ThriftClientUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss::utility {

bool verifySwSwitchRunState(
    const std::string& switchName,
    const SwitchRunState& expectedSwitchRunState) {
  auto switchRunStateMatches = [switchName, expectedSwitchRunState] {
    auto multiSwitchRunState = getMultiSwitchRunState(switchName);
    auto gotSwitchRunState = multiSwitchRunState.swSwitchRunState();
    return gotSwitchRunState == expectedSwitchRunState;
  };

  // Thrift client queries will throw exception while the Agent is
  // initializing. Thus, continue to retry while absorbing exceptions.
  return checkWithRetryErrorReturn(
      switchRunStateMatches,
      30 /* num retries */,
      std::chrono::milliseconds(5000) /* sleep between retries */,
      true /* retry on exception */);
}

} // namespace facebook::fboss::utility
