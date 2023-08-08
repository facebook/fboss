// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/experimental/FunctionScheduler.h>

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

namespace facebook::fboss {

class SwSwitchInitializer {
 public:
  explicit SwSwitchInitializer(SwSwitch* sw);
  virtual ~SwSwitchInitializer();
  void start();
  void start(HwSwitchCallback* callback);
  void stopFunctionScheduler();
  void waitForInitDone();
  void init(HwSwitchCallback* callback);

 protected:
  virtual void initImpl(HwSwitchCallback*) = 0;
  void initThread(HwSwitchCallback* callback);
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

  void stopServices();

  void stopAgent(bool setupWarmboot) override;

  int initAgent() override;
  int initAgent(HwSwitchCallback* callback);

 protected:
  virtual std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>
  getThrifthandlers() = 0;

  virtual void handleExitSignal();

  std::unique_ptr<SwSwitch> sw_;
  std::unique_ptr<SwSwitchInitializer> initializer_;
  std::unique_ptr<apache::thrift::ThriftServer> server_;
  folly::EventBase* eventBase_;
};

} // namespace facebook::fboss
