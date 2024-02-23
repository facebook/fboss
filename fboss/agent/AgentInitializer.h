// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

namespace facebook::fboss {
struct AgentInitializer {
  virtual ~AgentInitializer() = default;
  virtual int initAgent() = 0;
  virtual void stopAgent(bool setupWarmboot, bool gracefulExit) = 0;
};

} // namespace facebook::fboss
