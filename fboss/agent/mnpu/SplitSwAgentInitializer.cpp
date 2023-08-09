// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/mnpu/SplitSwAgentInitializer.h"
#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/TunManager.h"

#include <thread>

namespace facebook::fboss {

void SplitSwSwitchInitializer::initImpl(HwSwitchCallback* callback) {
  sw_->init(
      callback,
      nullptr,
      [](HwSwitchCallback* /*callback*/, bool /*failHwCallsOnWarmboot*/) {
        // TODO: need a different init for split sw-switch, as it there is no hw
        // agent
        return HwInitResult{};
      },
      []() { // TODO - return HwSwitchHandler for split sw-switch
        return nullptr;
      },
      setupFlags());
}

std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
SplitSwAgentInitializer::getThrifthandlers() {
  std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>> handlers;
  auto handler = std::make_shared<MultiSwitchThriftHandler>(sw_.get());
  handlers.push_back(handler);
  return handlers;
}

} // namespace facebook::fboss
