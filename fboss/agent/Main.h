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
#include <folly/io/async/AsyncSignalHandler.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TunManager.h"

#include <gflags/gflags.h>
#include <chrono>
#include <condition_variable>
#include <csignal>
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
  MonolithicSwSwitchInitializer(SwSwitch* sw, Platform* platform)
      : sw_(sw), platform_(platform) {}
  void start();
  void start(HwSwitchCallback* callback);
  void stopFunctionScheduler();
  void waitForInitDone();
  Platform* platform() {
    return platform_;
  }

 private:
  void initThread(HwSwitchCallback* callback);
  SwitchFlags setupFlags();
  void initImpl(HwSwitchCallback* callback);
  SwSwitch* sw_;
  Platform* platform_;
  std::unique_ptr<folly::FunctionScheduler> fs_;
  std::mutex initLock_;
  std::condition_variable initCondition_;
};

class SignalHandler : public folly::AsyncSignalHandler {
  typedef std::function<void()> StopServices;

 public:
  SignalHandler(
      folly::EventBase* eventBase,
      SwSwitch* sw,
      StopServices stopServices)
      : AsyncSignalHandler(eventBase), sw_(sw), stopServices_(stopServices) {
    registerSignalHandler(SIGINT);
    registerSignalHandler(SIGTERM);
  }

  void signalReceived(int /*signum*/) noexcept override;

 private:
  SwSwitch* sw_;
  StopServices stopServices_;
};

typedef std::unique_ptr<Platform> (
    *PlatformInitFn)(std::unique_ptr<AgentConfig>, uint32_t featuresDesired);

class MonolithicAgentInitializer {
 public:
  SwSwitch* sw() const {
    return sw_.get();
  }
  Platform* platform() const {
    return initializer_->platform();
  }
  MonolithicSwSwitchInitializer* initializer() const {
    return initializer_.get();
  }

  virtual ~MonolithicAgentInitializer() = default;
  void stopServices();
  void createSwitch(
      int argc,
      char** argv,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform);
  int initAgent();
  int initAgent(HwSwitchCallback* callback);
  void stopAgent(bool setupWarmboot);

  /*
   * API to all flag overrides for individual tests. Primarily
   * used for features which we don't want to enable for
   * all tests, but still want to tweak/test this behavior in
   * our test.
   */
  virtual void setCmdLineFlagOverrides() const {}

 private:
  std::unique_ptr<SwSwitch> sw_;
  std::unique_ptr<MonolithicSwSwitchInitializer> initializer_;
  std::unique_ptr<apache::thrift::ThriftServer> server_;
  folly::EventBase* eventBase_;
};

int fbossMain(
    int argc,
    char** argv,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform);

} // namespace facebook::fboss
