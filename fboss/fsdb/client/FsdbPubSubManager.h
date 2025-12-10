// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/Synchronized.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gtest/gtest_prod.h>
#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"
#include "fboss/fsdb/client/FsdbPatchSubscriber.h"
#include "fboss/fsdb/client/FsdbStateSubscriber.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

#include <mutex>
#include <string>
#include <vector>

DECLARE_int32(fsdbPathPublishQueueSize);

namespace facebook::fboss::fsdb {
class FsdbDeltaPublisher;
class FsdbStatePublisher;
class FsdbPatchPublisher;
class FsdbPubSubManager {
 public:
  explicit FsdbPubSubManager(
      const std::string& clientId,
      folly::EventBase* reconnectEvb = nullptr,
      folly::EventBase* subscriberEvb = nullptr,
      folly::EventBase* statsPublisherEvb = nullptr,
      folly::EventBase* statePublisherEvb = nullptr);
  ~FsdbPubSubManager();

  using Path = std::vector<std::string>;
  using PatchPath = std::map<SubscriptionKey, fboss::fsdb::RawOperPath>;
  using MultiPath = std::vector<Path>;

  /* Publisher create APIs */
  void createStateDeltaPublisher(
      const Path& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort);
  void createStatePathPublisher(
      const Path& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort,
      size_t queueSize = FLAGS_fsdbPathPublishQueueSize);
  void createStatePatchPublisher(
      const Path& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort);
  void createStatDeltaPublisher(
      const Path& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort);
  void createStatPathPublisher(
      const Path& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort,
      size_t queueSize = FLAGS_fsdbPathPublishQueueSize);
  void createStatPatchPublisher(
      const Path& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort);

  /* Publisher remove APIs */
  void removeStateDeltaPublisher(bool gracefulRestart = false);
  void removeStatePathPublisher(bool gracefulRestart = false);
  void removeStatePatchPublisher(bool gracefulRestart = false);
  void removeStatDeltaPublisher(bool gracefulRestart = false);
  void removeStatPathPublisher(bool gracefulRestart = false);
  void removeStatPatchPublisher(bool gracefulRestart = false);

  /* Publisher APIs */
  void publishState(OperDelta&& pubUnit);
  void publishState(OperState&& pubUnit);
  void publishState(Patch&& pubUnit);
  void publishStat(OperDelta&& pubUnit);
  void publishStat(OperState&& pubUnit);
  void publishStat(Patch&& pubUnit);

  /* Subscriber add APIs */
  std::string addStatDeltaSubscription(
      const Path& subscribePath,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      utils::ConnectionOptions&& connectionOptions =
          kDefaultConnectionOptions(),
      std::optional<FsdbStreamHeartbeatCb> heartbeatCb = std::nullopt);
  std::string addStatPathSubscription(
      const Path& subscribePath,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbStateSubscriber::FsdbOperStateUpdateCb operDeltaCb,
      utils::ConnectionOptions&& connectionOptions =
          kDefaultConnectionOptions(),
      std::optional<FsdbStreamHeartbeatCb> heartbeatCb = std::nullopt);
  /* multi path subscription */
  std::string addStateDeltaSubscription(
      const MultiPath& subscribePaths,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      utils::ConnectionOptions&& connectionOptions =
          kDefaultConnectionOptions());
  std::string addStatDeltaSubscription(
      const MultiPath& subscribePath,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      utils::ConnectionOptions&& connectionOptions =
          kDefaultConnectionOptions());
  std::string addStatPathSubscription(
      const MultiPath& subscribePath,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbExtStateSubscriber::FsdbOperStateUpdateCb operDeltaCb,
      utils::ConnectionOptions&& connectionOptions =
          kDefaultConnectionOptions());

  /* Apis that use ConnectionOptions */
  // TODO: change all above apis to use server options
  std::string addStatePathSubscription(
      const Path& subscribePath,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      utils::ConnectionOptions&& connectionOptions =
          kDefaultConnectionOptions(),
      std::optional<FsdbStreamHeartbeatCb> heartbeatCb = std::nullopt);
  std::string addStatePathSubscription(
      const MultiPath& subscribePaths,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbExtStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      utils::ConnectionOptions&& connectionOptions =
          kDefaultConnectionOptions(),
      const std::optional<std::string>& clientIdSuffix = std::nullopt);
  std::string addStateDeltaSubscription(
      const Path& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      utils::ConnectionOptions&& connectionOptions =
          kDefaultConnectionOptions(),
      std::optional<FsdbStreamHeartbeatCb> heartbeatCb = std::nullopt);
  std::string addStatePatchSubscription(
      const PatchPath& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbPatchSubscriber::FsdbOperPatchUpdateCb patchCb,
      utils::ConnectionOptions&& connectionOptions,
      std::optional<FsdbStreamHeartbeatCb> heartbeatCb = std::nullopt);
  std::string addStatePathSubscription(
      SubscriptionOptions&& subscriptionOptions,
      const Path& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      utils::ConnectionOptions&& connectionOptions,
      std::optional<FsdbStreamHeartbeatCb> heartbeatCb = std::nullopt);
  std::string addStatePathSubscription(
      SubscriptionOptions&& subscriptionOptions,
      const MultiPath& subscribePaths,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbExtStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      utils::ConnectionOptions&& connectionOptions,
      std::optional<FsdbStreamHeartbeatCb> heartbeatCb = std::nullopt);
  std::string addStateExtPathSubscription(
      const std::vector<ExtendedOperPath>& subscribePaths,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbExtStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      utils::ConnectionOptions&& connectionOptions);
  std::string addStatExtPathSubscription(
      const std::vector<ExtendedOperPath>& subscribePaths,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbExtStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      utils::ConnectionOptions&& connectionOptions);
  std::string addStateExtDeltaSubscription(
      const std::vector<ExtendedOperPath>& subscribePaths,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      utils::ConnectionOptions&& connectionOptions);
  std::string addStatExtDeltaSubscription(
      const std::vector<ExtendedOperPath>& subscribePaths,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      utils::ConnectionOptions&& connectionOptions);

  /* Subscriber remove APIs */
  void removeStateDeltaSubscription(
      const Path& subscribePath,
      const std::string& fsdbHost = "::1");
  void removeStatePatchSubscription(
      const Path& subscribePath,
      const std::string& fsdbHost);
  void removeStatePathSubscription(
      const Path& subscribePath,
      const std::string& fsdbHost = "::1");
  void removeStatDeltaSubscription(
      const Path& subscribePath,
      const std::string& fsdbHost = "::1");
  void removeStatPathSubscription(
      const Path& subscribePath,
      const std::string& fsdbHost = "::1");
  /* Multipath subscription remove apis*/
  void removeStateDeltaSubscription(
      const MultiPath& subscribePath,
      const std::string& fsdbHost = "::1");
  void removeStatePatchSubscription(
      const MultiPath& subscribePath,
      const std::string& fsdbHost = "::1");
  void removeStatePathSubscription(
      const MultiPath& subscribePath,
      const std::string& fsdbHost = "::1");
  void removeStatDeltaSubscription(
      const MultiPath& subscribePath,
      const std::string& fsdbHost = "::1");
  void removeStatPathSubscription(
      const MultiPath& subscribePath,
      const std::string& fsdbHost = "::1");
  void removeStateExtDeltaSubscription(
      const std::vector<ExtendedOperPath>& subscribePath,
      const std::string& fsdbHost = "::1");
  void removeStatExtDeltaSubscription(
      const std::vector<ExtendedOperPath>& subscribePath,
      const std::string& fsdbHost = "::1");

  void clearStateSubscriptions();
  void clearStatSubscriptions();

  FsdbStreamClient::State getStatePathSubsriptionState(
      const MultiPath& subscribePath,
      const std::string& fsdbHost = "::1") const;

  const std::vector<SubscriptionInfo> getSubscriptionInfo() const;

  size_t numSubscriptions() const {
    return statePath2Subscriber_.rlock()->size() +
        statPath2Subscriber_.rlock()->size();
  }

  FsdbDeltaPublisher* getDeltaPublisher(bool stats = false) {
    return stats ? statDeltaPublisher_.get() : stateDeltaPublisher_.get();
  }

  FsdbStatePublisher* getPathPublisher(bool stats = false) {
    return stats ? statPathPublisher_.get() : statePathPublisher_.get();
  }

  FsdbPatchPublisher* getPatchPublisher(bool stats = false) {
    return stats ? statPatchPublisher_.get() : statePatchPublisher_.get();
  }

  std::string getSubscriberStatsPrefix(bool stats, std::string& key) {
    auto& path2Subscriber =
        stats ? statPath2Subscriber_ : statePath2Subscriber_;
    auto path2SubscriberR = path2Subscriber.rlock();
    if (path2SubscriberR->find(key) != path2SubscriberR->end()) {
      return path2SubscriberR->find(key)->second->getCounterPrefix();
    }
    return "";
  }

  std::string getClientId() const {
    return clientId_;
  }

  static std::string subscriptionStateToString(FsdbStreamClient::State state);

 private:
  static utils::ConnectionOptions kDefaultConnectionOptions() {
    return utils::ConnectionOptions::defaultOptions<
        facebook::fboss::fsdb::FsdbService>();
  }
  // Publisher helpers
  template <typename PublisherT, typename PubUnitT>
  void publishImpl(PublisherT* publisher, PubUnitT&& pubUnit);
  template <typename PublisherT>
  std::unique_ptr<PublisherT> createPublisherImpl(
      const std::lock_guard<std::mutex>& /*lk*/,
      const std::vector<std::string>& publishPath,
      bool publishStats,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort,
      std::optional<size_t> publishQueueSize = std::nullopt) const;
  void stopPublisher(
      const std::lock_guard<std::mutex>& /*lk*/,
      std::unique_ptr<FsdbStreamClient> publisher);

  // Subscriber helpers
  template <typename PathElement>
  void removeSubscriptionImpl(
      const std::vector<PathElement>& subscribePath,
      const std::string& fsdbHost,
      SubscriptionType subscribeType,
      bool subscribeStats);

  template <typename SubscriberT, typename PathElement>
  std::string addSubscriptionImpl(
      const std::vector<PathElement>& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      typename SubscriberT::FsdbSubUnitUpdateCb subUnitAvailableCb,
      bool subscribeStats,
      utils::ConnectionOptions&& connectionOptions,
      const std::optional<std::string>& clientIdSuffix = std::nullopt,
      const std::optional<FsdbStreamHeartbeatCb>& heartbeatCb = std::nullopt);
  template <typename SubscriberT, typename PathElement>
  std::string addSubscriptionImpl(
      const std::map<SubscriptionKey, PathElement>& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      typename SubscriberT::FsdbSubUnitUpdateCb subUnitAvailableCb,
      bool subscribeStats,
      utils::ConnectionOptions&& connectionOptions,
      const std::optional<std::string>& clientIdSuffix = std::nullopt,
      const std::optional<FsdbStreamHeartbeatCb>& heartbeatCb = std::nullopt);
  template <typename SubscriberT, typename PathElement>
  std::string addSubscriptionImpl(
      SubscriptionOptions&& subscriptionOptions,
      const std::vector<PathElement>& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      typename SubscriberT::FsdbSubUnitUpdateCb subUnitAvailableCb,
      utils::ConnectionOptions&& connectionOptions,
      const std::optional<FsdbStreamHeartbeatCb>& heartbeatCb = std::nullopt);

  const std::string clientId_;

  // local threads are only needed when there are no external eventbases
  std::unique_ptr<folly::ScopedEventBaseThread> reconnectEvbThread_{nullptr};
  std::unique_ptr<folly::ScopedEventBaseThread> subscriberEvbThread_{nullptr};
  std::unique_ptr<folly::ScopedEventBaseThread> statsPublisherStreamEvbThread_{
      nullptr};
  std::unique_ptr<folly::ScopedEventBaseThread> statePublisherStreamEvbThread_{
      nullptr};

  // eventbase pointers from either external/internal threads
  folly::EventBase* reconnectEvb_;
  folly::EventBase* subscriberEvb_;
  folly::EventBase* statsPublisherEvb_;
  folly::EventBase* statePublisherEvb_;

  std::mutex publisherMutex_;
  // State Publishers
  std::unique_ptr<FsdbDeltaPublisher> stateDeltaPublisher_;
  std::unique_ptr<FsdbStatePublisher> statePathPublisher_;
  std::unique_ptr<FsdbPatchPublisher> statePatchPublisher_;
  // Stat Publishers
  std::unique_ptr<FsdbDeltaPublisher> statDeltaPublisher_;
  std::unique_ptr<FsdbStatePublisher> statPathPublisher_;
  std::unique_ptr<FsdbPatchPublisher> statPatchPublisher_;
  // Subscribers
  folly::Synchronized<
      std::unordered_map<std::string, std::unique_ptr<FsdbSubscriberBase>>>
      statePath2Subscriber_;
  folly::Synchronized<
      std::unordered_map<std::string, std::unique_ptr<FsdbSubscriberBase>>>
      statPath2Subscriber_;

// per class placeholder for test code injection
// only need to be setup once here
#ifdef FsdbPubSubManager_TEST_FRIENDS
  FsdbPubSubManager_TEST_FRIENDS;
#endif
};
} // namespace facebook::fboss::fsdb
