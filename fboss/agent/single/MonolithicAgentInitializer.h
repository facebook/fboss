// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <memory>

#include <folly/executors/FunctionScheduler.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/HwAgent.h"
#include "fboss/agent/SwAgentInitializer.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TunManager.h"

#include <gflags/gflags.h>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <functional>
#include <future>
#include <mutex>
#include <string>

namespace apache::thrift {
class ThriftServer;
} // namespace apache::thrift

namespace facebook::fboss {

struct AgentConfig;
class Platform;

class MonolithicSwSwitchInitializer : public SwSwitchInitializer {
 public:
  MonolithicSwSwitchInitializer(SwSwitch* sw, HwAgent* hwAgent)
      : SwSwitchInitializer(sw), hwAgent_(hwAgent) {}
  Platform* platform() {
    return hwAgent_->getPlatform();
  }

 private:
  void initImpl(
      HwSwitchCallback* callback,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE) override;
  HwAgent* hwAgent_;
};

class MonolithicAgentInitializer : public SwAgentInitializer {
 public:
  MonolithicAgentInitializer() = default;
  MonolithicAgentInitializer(
      std::unique_ptr<AgentConfig> config,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform);
  Platform* platform() const {
    return hwAgent_->getPlatform();
  }

  virtual ~MonolithicAgentInitializer() override = default;
  void createSwitch(
      std::unique_ptr<AgentConfig> config,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform);

  /*
   * API to all flag overrides for individual tests. Primarily
   * used for features which we don't want to enable for
   * all tests, but still want to tweak/test this behavior in
   * our test.
   */
  virtual void setCmdLineFlagOverrides() const {}

  void handleExitSignal(bool gracefulExit) override;

  /*
   * MonoithicAgentInitializer overrides stopServices to stop the HwAgent first,
   * particularly the WedgePlatform BcmSwitch for high frequency stats
   * collection.
   */
  void stopServices() override;

  std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
  getThrifthandlers() override;

 private:
  std::unique_ptr<HwAgent> hwAgent_;
};

} // namespace facebook::fboss
