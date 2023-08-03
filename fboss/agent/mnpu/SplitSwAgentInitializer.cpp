// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/mnpu/SplitSwAgentInitializer.h"

#include <thread>

namespace facebook::fboss {

int SplitSwAgentInitializer::initAgent() {
  auto initThread = std::thread([this]() { dummyEvb_.loopForever(); });
  initThread.join();
  return 0;
}

void SplitSwAgentInitializer::stopAgent(bool /*setupWarmboot*/) {
  dummyEvb_.runInEventBaseThread([=]() { dummyEvb_.terminateLoopSoon(); });
}

} // namespace facebook::fboss
