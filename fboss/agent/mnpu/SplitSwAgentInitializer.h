// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/SwAgentInitializer.h"

#include <folly/io/async/EventBase.h>

namespace facebook::fboss {

class SwitchState;

class SplitSwSwitchInitializer : public SwSwitchInitializer {
 public:
  explicit SplitSwSwitchInitializer(SwSwitch* sw) : SwSwitchInitializer(sw) {}

 private:
  void initImpl(HwSwitchCallback* callback) override;
};

class SplitSwAgentInitializer : public SwAgentInitializer {
 public:
  SplitSwAgentInitializer();
  virtual ~SplitSwAgentInitializer() override {}

  std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
  getThrifthandlers() override;

  void handleExitSignal() override;

 private:
  AgentDirectoryUtil agentDirectoryUtil_;
};

} // namespace facebook::fboss
