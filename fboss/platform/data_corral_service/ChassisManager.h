// Copyright 2014-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/platform/data_corral_service/FruModule.h>
#include <folly/io/async/AsyncTimeout.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <folly/system/ThreadName.h>

namespace facebook::fboss::platform::data_corral_service {

class ChassisMonitor : private folly::AsyncTimeout {
 public:
  ChassisMonitor(
      int refreshInterval,
      folly::EventBase* evb,
      std::function<void()> func)
      : AsyncTimeout(evb),
        evb_(evb),
        refreshInterval_(refreshInterval),
        func_(func) {
    evb_->runInEventBaseThread([this]() { scheduleFirstRefresh(); });
  }

  ~ChassisMonitor() override {
    evb_->runImmediatelyOrRunInEventBaseThreadAndWait(
        [this]() { cancelTimeout(); });
  }

 private:
  void timeoutExpired() noexcept override;
  void scheduleFirstRefresh();
  folly::EventBase* evb_;
  const std::chrono::milliseconds refreshInterval_;
  std::function<void()> func_;
  std::atomic<std::chrono::time_point<std::chrono::steady_clock>> lastTime_{
      std::chrono::steady_clock::time_point::min()};
};

class ChassisManager {
  // Refresh chassis modules regularly to detect events and program chassis
  // Child class implement platform dependent logics
 public:
  explicit ChassisManager(int refreshInterval) {
    refreshInterval_ = refreshInterval;
  }

  void init() {
    initModules();
    startThread();
    auto func = [this]() { refreshFruModules(); };
    monitor_ = std::make_unique<ChassisMonitor>(refreshInterval_, &evb_, func);
  }

  void initThread();

  virtual void initModules() {
    // populate fruModules and system level settings
  }

  virtual void refreshFruModules() {
    for (auto& fruModule : fruModules_) {
      fruModule.second->refresh();
    }
    programChassis();
  }

  virtual void programChassis() {
    // program chassis system based on Fru state,
    // e.g. system LED
  }

  void startThread() {
    thread_.reset(new std::thread([=] { this->threadLoop(); }));
  }

  void stopThread() {
    evb_.runInEventBaseThread([this] { evb_.terminateLoopSoon(); });
    thread_->join();
  }

  void threadLoop() {
    initThread();
    evb_.loopForever();
  }

  virtual ~ChassisManager() {
    if (monitor_) {
      monitor_.reset();
      stopThread();
    }
  }

 protected:
  int refreshInterval_;
  std::unordered_map<std::string, std::unique_ptr<FruModule>> fruModules_;
  std::unique_ptr<std::thread> thread_;
  folly::EventBase evb_;
  std::unique_ptr<ChassisMonitor> monitor_;
};

enum ChassisLedColor {
  OFF = 0,
  RED = 1,
  GREEN = 2,
  BLUE = 3,
  AMBER = 4,
};

} // namespace facebook::fboss::platform::data_corral_service
