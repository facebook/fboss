// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <memory>

#include <folly/experimental/FunctionScheduler.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/agent/AgentInitializer.h"
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

namespace apache {
namespace thrift {
class ThriftServer;
}
} // namespace apache

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
  void initImpl(HwSwitchCallback* callback) override;
  HwAgent* hwAgent_;
};

class MonolithicAgentInitializer : public AgentInitializer {
 public:
  MonolithicAgentInitializer() {}
  MonolithicAgentInitializer(
      std::unique_ptr<AgentConfig> config,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform);
  SwSwitch* sw() const {
    return sw_.get();
  }
  Platform* platform() const {
    return hwAgent_->getPlatform();
  }
  MonolithicSwSwitchInitializer* initializer() const {
    return initializer_.get();
  }

  virtual ~MonolithicAgentInitializer() override {}
  void stopServices();
  void createSwitch(
      std::unique_ptr<AgentConfig> config,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform);
  int initAgent() override;
  int initAgent(HwSwitchCallback* callback);
  void stopAgent(bool setupWarmboot) override;

  /*
   * API to all flag overrides for individual tests. Primarily
   * used for features which we don't want to enable for
   * all tests, but still want to tweak/test this behavior in
   * our test.
   */
  virtual void setCmdLineFlagOverrides() const {}

 private:
  std::unique_ptr<HwAgent> hwAgent_;
  std::unique_ptr<SwSwitch> sw_;
  std::unique_ptr<MonolithicSwSwitchInitializer> initializer_;
  std::unique_ptr<apache::thrift::ThriftServer> server_;
  folly::EventBase* eventBase_;
};

} // namespace facebook::fboss
