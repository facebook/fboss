// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/SwAgentInitializer.h"

#include <folly/io/async/EventBase.h>

namespace facebook::fboss {

class SwitchState;
class MultiSwitchThriftHandler;

class SplitSwSwitchInitializer : public SwSwitchInitializer {
 public:
  explicit SplitSwSwitchInitializer(SwSwitch* sw) : SwSwitchInitializer(sw) {}

 private:
  void initImpl(
      HwSwitchCallback* callback,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE) override;
};

class SplitSwAgentInitializer : public SwAgentInitializer {
 public:
  SplitSwAgentInitializer();
  virtual ~SplitSwAgentInitializer() override {}

  std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
  getThrifthandlers() override;

  void handleExitSignal(bool gracefulExit) override;

  void stopAgent(bool setupWarmboot, bool gracefulExit) override;

 private:
  void exitForColdBoot();
  void exitForWarmBoot(bool gracefulExit);
  AgentDirectoryUtil agentDirectoryUtil_;
  MultiSwitchThriftHandler* multiSwitchThriftHandler_{nullptr};
};

} // namespace facebook::fboss
