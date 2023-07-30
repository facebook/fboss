/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <memory>

#include <folly/experimental/FunctionScheduler.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/HwAgent.h"
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

class AgentConfig;
class Platform;

class MonolithicSwSwitchInitializer {
 public:
  MonolithicSwSwitchInitializer(SwSwitch* sw, HwAgent* hwAgent)
      : sw_(sw), hwAgent_(hwAgent) {}
  void start();
  void start(HwSwitchCallback* callback);
  void stopFunctionScheduler();
  void waitForInitDone();
  Platform* platform() {
    return hwAgent_->getPlatform();
  }

 private:
  void initThread(HwSwitchCallback* callback);
  SwitchFlags setupFlags();
  void initImpl(HwSwitchCallback* callback);
  SwSwitch* sw_;
  HwAgent* hwAgent_;
  std::unique_ptr<folly::FunctionScheduler> fs_;
  std::mutex initLock_;
  std::condition_variable initCondition_;
};

class MonolithicAgentSignalHandler : public SignalHandler {
 public:
  MonolithicAgentSignalHandler(
      folly::EventBase* eventBase,
      SwSwitch* sw,
      SignalHandler::StopServices stopServices)
      : SignalHandler(eventBase, stopServices), sw_(sw) {}

  void signalReceived(int /*signum*/) noexcept override;

 private:
  SwSwitch* sw_;
};

struct AgentInitializer {
  virtual ~AgentInitializer() = default;
  virtual int initAgent() = 0;
  virtual void stopAgent(bool setupWarmboot) = 0;
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

int fbossMain(
    int argc,
    char** argv,
    std::unique_ptr<AgentInitializer> initializer);

} // namespace facebook::fboss
