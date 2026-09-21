// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/Synchronized.h>
#include <folly/synchronization/Baton.h>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace facebook::fboss::fsdb {
class FsdbPubSubManager;
namespace test {
class FsdbTestServer;
}
} // namespace facebook::fboss::fsdb

namespace facebook::fboss::fsdb::test {

class FsdbTestCgoClient {
 public:
  explicit FsdbTestCgoClient(std::string clientId) noexcept;
  ~FsdbTestCgoClient();

  // Server configuration
  void setServer(std::string hostIp, int32_t fsdbPort);
  void startInProcessServer();
  void stopInProcessServer();
  int32_t getServerPort() const;

  // Path subscription (via FsdbPubSubManager)
  void subscribeStatePath(std::vector<std::string> path);
  void unsubscribeStatePath();

  // Delta subscription (via FsdbPubSubManager)
  void subscribeStateDelta(std::vector<std::string> path);
  void unsubscribeStateDelta();

  // Patch subscription (via FsdbSubManager / FsdbCowStateSubManager)
  void subscribeStatePatch(std::vector<std::string> path);
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

  // Path publisher (via FsdbPubSubManager - simpler than Patch publisher)
  void connectPublisher();
  void disconnectPublisher();
  bool isPublisherConnected() const;
  bool waitForPublisherConnected(int64_t timeoutMs);

  // Generic publish (for raw data - may not work with all FSDB setups)
  void publishTestState(std::vector<std::string> path, std::string jsonValue);

  // Typed data publishing with proper Thrift COW serialization
  // Publish to agent.config.defaultCommandLineArgs (map<string, string>)
  void publishAgentConfigCmdLineArgs(std::string key, std::string value);

  // Publish to agent.switchState.portMaps (map<int, map<int, PortInfo>>)
  void publishAgentSwitchStatePortInfo(
      int64_t switchId,
      int64_t portId,
      std::string portName,
      int32_t operState);

  // Check if publisher is ready for typed publishing
  bool waitForPublisherReady(int64_t timeoutMs);

 private:
  std::string clientId_;
  std::string hostIp_{"::1"};
  int32_t fsdbPort_{5908};

  // FsdbPubSubManager for Path and Delta subscriptions
  std::unique_ptr<FsdbPubSubManager> pubSubManager_;

  // Separate FsdbPubSubManager for publishing with "agent" as client ID
  std::unique_ptr<FsdbPubSubManager> publisherPubSubManager_;

  // FsdbSubManager for Patch subscriptions (type-erased)
  std::unique_ptr<void, void (*)(void*)> subManager_;

  // In-process test server
  std::unique_ptr<FsdbTestServer> testServer_;

  // Publisher state tracking
  std::atomic<bool> publisherConnected_{false};
  folly::Baton<> publisherConnectedBaton_;
  std::vector<std::string> publisherPath_;
  bool publisherActive_{false};

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
};

} // namespace facebook::fboss::fsdb::test
