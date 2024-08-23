// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/platforms/sai/SaiPlatformInit.h"
#include "fboss/agent/test/integration_tests/AgentIntegrationTest.h"

int main(int argc, char* argv[]) {
  std::optional<facebook::fboss::cfg::StreamType> streamType = std::nullopt;
#if defined(TAJO_SDK_GTE_24_4_90)
  streamType = facebook::fboss::cfg::StreamType::UNICAST;
#endif
  return facebook::fboss::agentIntegrationTestMain(
      argc, argv, facebook::fboss::initSaiPlatform, streamType);
}
