// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/mnpu/SplitSwAgentInitializer.h"
#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/agent/TunManager.h"
#include "fboss/agent/mnpu/NonMonolithicHwSwitchHandler.h"
#include "fboss/lib/CommonFileUtils.h"

#include <thread>

namespace facebook::fboss {

void SplitSwSwitchInitializer::initImpl(HwSwitchCallback* callback) {
  // this blocks until at least one hardware switch is up
  sw_->init(setupFlags());
}

SplitSwAgentInitializer::SplitSwAgentInitializer() {
  sw_ = std::make_unique<SwSwitch>(
      [](const SwitchID& switchId, const cfg::SwitchInfo& info, SwSwitch* sw) {
        return std::make_unique<MultiSwitchHwSwitchHandler>(switchId, info, sw);
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

void SplitSwAgentInitializer::handleExitSignal(bool gracefulExit) {
  if (!sw_->isInitialized()) {
    XLOG(WARNING)
        << "[Exit] Signal received before initializing sw switch, waiting for initialization to finish.";
  }
  initializer()->waitForInitDone();
  {
    auto exitForColdBootFile =
        agentDirectoryUtil_.exitSwSwitchForColdBootFile();
    SCOPE_EXIT {
      removeFile(exitForColdBootFile);
    };
    if (checkFileExists(exitForColdBootFile)) {
      stopAgent(false, gracefulExit);
    } else {
      SwAgentInitializer::handleExitSignal(gracefulExit);
    }
  }
  if (gracefulExit) {
    exit(0);
  } else {
    exit(1);
  }
}

void SplitSwAgentInitializer::stopAgent(bool setupWarmboot, bool gracefulExit) {
  if (setupWarmboot) {
    handleExitSignal(gracefulExit);
  } else {
    sw_->stop(false /* gracefulStop */, true /* revertToMinAlpmState */);
    sw_->getHwSwitchHandler()->stop();
    stopServices();
    initializer_.reset();
  }
}

} // namespace facebook::fboss
