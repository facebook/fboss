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

#include <mutex>
#include <string>
#include <vector>

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
      int32_t fsdbPort = FLAGS_fsdbPort);
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
      int32_t fsdbPort = FLAGS_fsdbPort);
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
  void addStatDeltaSubscription(
      const Path& subscribePath,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      FsdbStreamClient::ServerOptions&& serverOptions =
          kDefaultServerOptions());
  void addStatPathSubscription(
      const Path& subscribePath,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbStateSubscriber::FsdbOperStateUpdateCb operDeltaCb,
      FsdbStreamClient::ServerOptions&& serverOptions =
          kDefaultServerOptions());
  /* multi path subscription */
  void addStateDeltaSubscription(
      const MultiPath& subscribePaths,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      FsdbStreamClient::ServerOptions&& serverOptions =
          kDefaultServerOptions());
  void addStatDeltaSubscription(
      const MultiPath& subscribePath,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      FsdbStreamClient::ServerOptions&& serverOptions =
          kDefaultServerOptions());
  void addStatPathSubscription(
      const MultiPath& subscribePath,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbExtStateSubscriber::FsdbOperStateUpdateCb operDeltaCb,
      FsdbStreamClient::ServerOptions&& serverOptions =
          kDefaultServerOptions());

  /* Apis that use ServerOptions */
  // TODO: change all above apis to use server options
  void addStatePathSubscription(
      const Path& subscribePath,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      FsdbStreamClient::ServerOptions&& serverOptions =
          kDefaultServerOptions());
  void addStatePathSubscription(
      const MultiPath& subscribePaths,
      SubscriptionStateChangeCb subscriptionStateChangeCb,
      FsdbExtStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      FsdbStreamClient::ServerOptions&& serverOptions = kDefaultServerOptions(),
      const std::optional<std::string>& clientIdSuffix = std::nullopt);
  void addStateDeltaSubscription(
      const Path& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      FsdbStreamClient::ServerOptions&& serverOptions =
          kDefaultServerOptions());
  void addStatePatchSubscription(
      const PatchPath& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbPatchSubscriber::FsdbOperPatchUpdateCb patchCb,
      FsdbStreamClient::ServerOptions&& serverOptions);
  void addStatePathSubscription(
      SubscriptionOptions&& subscriptionOptions,
      const Path& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      FsdbStreamClient::ServerOptions&& serverOptions);
  void addStatePathSubscription(
      SubscriptionOptions&& subscriptionOptions,
      const MultiPath& subscribePaths,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbExtStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      FsdbStreamClient::ServerOptions&& serverOptions);
  void addStateExtPathSubscription(
      const std::vector<ExtendedOperPath>& subscribePaths,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbExtStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      FsdbStreamClient::ServerOptions&& serverOptions);
  void addStatExtPathSubscription(
      const std::vector<ExtendedOperPath>& subscribePaths,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbExtStateSubscriber::FsdbOperStateUpdateCb operStateCb,
      FsdbStreamClient::ServerOptions&& serverOptions);
  void addStateExtDeltaSubscription(
      const std::vector<ExtendedOperPath>& subscribePaths,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      FsdbStreamClient::ServerOptions&& serverOptions);
  void addStatExtDeltaSubscription(
      const std::vector<ExtendedOperPath>& subscribePaths,
      SubscriptionStateChangeCb stateChangeCb,
      FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      FsdbStreamClient::ServerOptions&& serverOptions);

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

  std::string getClientId() const {
    return clientId_;
  }

  static std::string subscriptionStateToString(FsdbStreamClient::State state);

 private:
  static FsdbStreamClient::ServerOptions kDefaultServerOptions() {
    return FsdbStreamClient::ServerOptions("::1", FLAGS_fsdbPort);
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
      int32_t fsdbPort) const;
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
  void addSubscriptionImpl(
      const std::vector<PathElement>& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      typename SubscriberT::FsdbSubUnitUpdateCb subUnitAvailableCb,
      bool subscribeStats,
      FsdbStreamClient::ServerOptions&& serverOptions,
      const std::optional<std::string>& clientIdSuffix = std::nullopt);
  template <typename SubscriberT, typename PathElement>
  void addSubscriptionImpl(
      const std::map<SubscriptionKey, PathElement>& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      typename SubscriberT::FsdbSubUnitUpdateCb subUnitAvailableCb,
      bool subscribeStats,
      FsdbStreamClient::ServerOptions&& serverOptions,
      const std::optional<std::string>& clientIdSuffix = std::nullopt);
  template <typename SubscriberT, typename PathElement>
  void addSubscriptionImpl(
      SubscriptionOptions&& subscriptionOptions,
      const std::vector<PathElement>& subscribePath,
      SubscriptionStateChangeCb stateChangeCb,
      typename SubscriberT::FsdbSubUnitUpdateCb subUnitAvailableCb,
      FsdbStreamClient::ServerOptions&& serverOptions);

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
