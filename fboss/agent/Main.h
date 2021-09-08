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

class Initializer {
 public:
  Initializer(SwSwitch* sw, Platform* platform)
      : sw_(sw), platform_(platform) {}
  void start();
  void stopFunctionScheduler();
  void waitForInitDone();

 private:
  void initThread();
  SwitchFlags setupFlags();
  void initImpl();
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

class AgentInitializer {
 protected:
  SwSwitch* sw() const {
    return sw_.get();
  }
  Platform* platform() const {
    return sw_->getPlatform();
  }
  Initializer* initializer() const {
    return initializer_.get();
  }

  template <typename CONDITION_FN>
  void checkWithRetry(
      CONDITION_FN condition,
      int retries = 10,
      std::chrono::duration<uint32_t, std::milli> msBetweenRetry =
          std::chrono::milliseconds(1000)) const {
    while (retries--) {
      if (condition()) {
        return;
      }
      std::this_thread::sleep_for(msBetweenRetry);
    }
    throw FbossError("Verify with retry failed, condition was never satisfied");
  }

 public:
  virtual ~AgentInitializer() = default;
  void stopServices();
  void createSwitch(
      int argc,
      char** argv,
      uint32_t hwFeaturesDesired,
      PlatformInitFn initPlatform);
  int initAgent();
  void stopAgent(bool setupWarmboot);

 private:
  virtual void preAgentInit() {}
  std::unique_ptr<SwSwitch> sw_;
  std::unique_ptr<Initializer> initializer_;
  std::unique_ptr<apache::thrift::ThriftServer> server_;
  folly::EventBase* eventBase_;
};

int fbossMain(
    int argc,
    char** argv,
    uint32_t hwFeaturesDesired,
    PlatformInitFn initPlatform);
void fbossFinalize();
void setVersionInfo();

} // namespace facebook::fboss
