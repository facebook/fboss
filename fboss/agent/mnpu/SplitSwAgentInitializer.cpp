// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/mnpu/SplitSwAgentInitializer.h"
#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/mnpu/NonMonolithicHwSwitchHandler.h"

#include <thread>

namespace facebook::fboss {

void SplitSwSwitchInitializer::initImpl(HwSwitchCallback* callback) {
  // this blocks until at least one hardware switch is up
  sw_->init(setupFlags());
}

SplitSwAgentInitializer::SplitSwAgentInitializer() {
  sw_ = std::make_unique<SwSwitch>(
      [](const SwitchID& switchId, const cfg::SwitchInfo& info, SwSwitch* sw) {
        return std::make_unique<NonMonolithicHwSwitchHandler>(
            switchId, info, sw);
      },
      &agentDirectoryUtil_,
      true /* supportsAddRemovePort */,
      nullptr /* config */);
  initializer_ = std::make_unique<SplitSwSwitchInitializer>(sw_.get());
}

std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
SplitSwAgentInitializer::getThrifthandlers() {
  std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>> handlers;
  auto handler = std::make_shared<MultiSwitchThriftHandler>(sw_.get());
  handlers.push_back(handler);
  return handlers;
}

void SplitSwAgentInitializer::handleExitSignal() {
  if (!sw_->isInitialized()) {
    XLOG(WARNING)
        << "[Exit] Signal received before initializing sw switch, waiting for initialization to finish.";
  }
  initializer()->waitForInitDone();
  SwAgentInitializer::handleExitSignal();
  exit(0);
}

} // namespace facebook::fboss
