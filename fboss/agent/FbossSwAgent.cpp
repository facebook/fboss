// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/Main.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/mnpu/SplitSwAgentInitializer.h"

using namespace facebook::fboss;

int main(int argc, char* argv[]) {
  auto config = fbossCommonInit(argc, argv);
  auto fbossInitializer = std::make_unique<SplitSwAgentInitializer>();
  return facebook::fboss::fbossMain(argc, argv, std::move(fbossInitializer));
}
