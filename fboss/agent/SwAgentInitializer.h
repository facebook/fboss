// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/executors/FunctionScheduler.h>

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/agent/AgentInitializer.h"
#include "fboss/agent/CommonInit.h"
#include "fboss/agent/SwSwitch.h"

#include <gflags/gflags.h>
#include <condition_variable>
#include <mutex>

DECLARE_bool(tun_intf);
DECLARE_bool(enable_lacp);
DECLARE_bool(enable_lldp);
DECLARE_bool(publish_boot_type);
DECLARE_bool(enable_macsec);
DECLARE_bool(enable_stats_update_thread);

namespace facebook::fboss {

class SwSwitchInitializer {
 public:
  explicit SwSwitchInitializer(SwSwitch* sw);
  virtual ~SwSwitchInitializer();
  void start();
  void start(
      HwSwitchCallback* callback,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE);
  void stopFunctionScheduler();
  void waitForInitDone();
  void init(
      HwSwitchCallback* callback,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE);

 protected:
  virtual void initImpl(
      HwSwitchCallback*,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE) = 0;
  void initThread(
      HwSwitchCallback* callback,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE);
  SwitchFlags setupFlags();

  SwSwitch* sw_;
  std::unique_ptr<folly::FunctionScheduler> fs_;
  std::mutex initLock_;
  std::condition_variable initCondition_;
  std::unique_ptr<std::thread> initThread_;
};

class SwAgentSignalHandler : public SignalHandler {
 public:
  SwAgentSignalHandler(
      folly::EventBase* eventBase,
      SwSwitch* sw,
      SignalHandler::StopServices stopServices)
      : SignalHandler(eventBase, std::move(stopServices)), sw_(sw) {}

  void signalReceived(int /*signum*/) noexcept override;

 private:
  SwSwitch* sw_;
};

class SwAgentInitializer : public AgentInitializer {
 public:
  SwAgentInitializer() {}

  SwSwitch* sw() const {
    return sw_.get();
  }

  SwSwitchInitializer* initializer() const {
    return initializer_.get();
  }

  // In case of gtest failures, we need to do an unclean exit to flag failures.
  // Hence control that via the gracefulExit flag
  void stopAgent(bool setupWarmboot, bool gracefulExit) override;

  int initAgent() override;
  int initAgent(
      HwSwitchCallback* callback,
      const HwWriteBehavior& hwWriteBehavior = HwWriteBehavior::WRITE);

 protected:
  virtual std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
  getThrifthandlers() = 0;

  std::unique_ptr<SwSwitch> sw_;
  std::unique_ptr<SwSwitchInitializer> initializer_;
  virtual void handleExitSignal(bool gracefulExit) = 0;

  void stopServer();
  void stopServices();
  void waitForServerStopped();

 private:
  std::unique_ptr<apache::thrift::ThriftServer> server_;
  FbossEventBase* eventBase_;

  std::atomic<bool> serverStarted_{false};
  std::mutex serverStopMutex_;
  std::condition_variable serverStopCV_;
};

} // namespace facebook::fboss
