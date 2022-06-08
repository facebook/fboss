// Copyright 2014-present Facebook. All Rights Reserved.

#include <fboss/platform/data_corral_service/FruModule.h>
#include <folly/io/async/AsyncTimeout.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <folly/system/ThreadName.h>

namespace facebook::fboss::platform::data_corral_service {

class ChassisManager : private folly::AsyncTimeout {
  // Refresh chassis modules regularly to detect events and program chassis
  // Child class implement platform dependent logics
 public:
  explicit ChassisManager(int refreshInterval) {
    refreshInterval_ = std::chrono::milliseconds(refreshInterval);
    initFruModules();
    startThread();
    evb_.runInEventBaseThread([this]() { scheduleFirstRefresh(); });
  }

  virtual void initFruModules() {
    // populate fruModules_;
  }

  virtual void refreshFruModules() {
    for (auto fruModule : fruModules_) {
      fruModule.refresh();
    }
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
    folly::StringPiece name = "ChassisManagerThread";
    folly::setThreadName(name);
    google::SetLogThreadName(name.str());
    evb_.loopForever();
  }

  virtual ~ChassisManager() override {
    evb_.runImmediatelyOrRunInEventBaseThreadAndWait(
        [this]() { cancelTimeout(); });
    stopThread();
  }

 protected:
  void timeoutExpired() noexcept override;
  void scheduleFirstRefresh();

  std::vector<FruModule> fruModules_;
  std::unique_ptr<std::thread> thread_;
  folly::EventBase evb_;
  std::chrono::milliseconds refreshInterval_;
  std::atomic<std::chrono::time_point<std::chrono::steady_clock>> lastTime_{
      std::chrono::steady_clock::time_point::min()};
};

} // namespace facebook::fboss::platform::data_corral_service
