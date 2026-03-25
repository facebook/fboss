// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/Synchronized.h>
#include <folly/synchronization/Baton.h>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace facebook::fboss::fsdb {
class FsdbPubSubManager;
} // namespace facebook::fboss::fsdb

namespace facebook::fboss::fsdb {

class FsdbCgoSubscriber {
 public:
  explicit FsdbCgoSubscriber(
      std::string clientId,
      std::string hostIp,
      int32_t fsdbPort) noexcept;
  ~FsdbCgoSubscriber();

  // Generic hooks
  void init();
  void cleanup();

  // Path subscription (via FsdbPubSubManager)
  void subscribeStatePath(std::vector<std::string> path);
  void subscribeStatPath(std::vector<std::string> path);
  void unsubscribeStatePath();
  void unsubscribeStatPath();

  // Delta subscription (via FsdbPubSubManager)
  void subscribeStateDelta(std::vector<std::string> path);
  void subscribeStatDelta(std::vector<std::string> path);
  void unsubscribeStateDelta();
  void unsubscribeStatDelta();

  // Patch subscription (via FsdbCowStateSubManager / FsdbCowStatsSubManager)
  void subscribeStatePatch(
      std::vector<std::vector<std::string>> paths,
      bool isStats);
  void unsubscribeStatePatch();

  // Unsubscribe all
  void unsubscribeAll();

  // State observation (subscriptionType: 0=Path, 1=Delta, 2=Patch)
  int32_t getSubscriptionState(int32_t subscriptionType) const;
  bool isInitialSyncComplete(int32_t subscriptionType) const;

  // Update consumption
  bool waitForUpdate(int64_t timeoutMs);
  int64_t getPathUpdateCount() const;
  int64_t getDeltaUpdateCount() const;
  int64_t getPatchUpdateCount() const;
  std::string getLastPathUpdateContent() const;
  std::string getLastDeltaUpdateContent() const;
  std::string getLastPatchUpdateContent() const;

 private:
  std::string clientId_;
  std::string hostIp_;
  int32_t fsdbPort_;

  // FsdbPubSubManager for Path and Delta subscriptions
  std::unique_ptr<FsdbPubSubManager> pubSubManager_;

  // FsdbSubManager for Patch subscriptions (type-erased with correct deleter
  // per type)
  std::function<void()> subManagerCleanup_;

  // State tracking
  struct SubscriptionTracker {
    std::atomic<int32_t> state{0};
    std::atomic<bool> initialSyncComplete{false};
    std::atomic<int64_t> updateCount{0};
    folly::Synchronized<std::string> lastContent;
  };
  SubscriptionTracker pathTracker_;
  SubscriptionTracker deltaTracker_;
  SubscriptionTracker patchTracker_;

  // Update notification
  folly::Baton<> updateBaton_;

  // Subscription handles
  std::string pathSubscriptionHandle_;
  std::string deltaSubscriptionHandle_;
  std::vector<std::string> pathSubscribePath_;
  std::vector<std::string> deltaSubscribePath_;
  bool patchSubscriptionActive_{false};
  bool patchIsStats_{false};
};

} // namespace facebook::fboss::fsdb
