// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once
#include <folly/Synchronized.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include "fboss/fsdb/Flags.h"
#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"
#include "fboss/fsdb/client/FsdbStateSubscriber.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <mutex>
#include <string>
#include <vector>

namespace facebook::fboss::fsdb {
class FsdbDeltaPublisher;
class FsdbStatePublisher;
class FsdbPubSubManager {
 public:
  explicit FsdbPubSubManager(const std::string& clientId);
  ~FsdbPubSubManager();

  /* Publisher create APIs */
  void createStateDeltaPublisher(
      const std::vector<std::string>& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort);
  void createStatePathPublisher(
      const std::vector<std::string>& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort);
  void createStatDeltaPublisher(
      const std::vector<std::string>& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort);
  void createStatPathPublisher(
      const std::vector<std::string>& publishPath,
      FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
      int32_t fsdbPort = FLAGS_fsdbPort);

  /* Publisher remove APIs */
  void removeStateDeltaPublisher();
  void removeStatePathPublisher();
  void removeStatDeltaPublisher();
  void removeStatPathPublisher();

  /* Publisher APIs */
  void publishState(OperDelta&& pubUnit);
  void publishState(OperState&& pubUnit);
  void publishStat(OperDelta&& pubUnit);
  void publishStat(OperState&& pubUnit);

  /* Subscriber add APIs */
  void addStateDeltaSubscription(
      const std::vector<std::string>& subscribePath,
      FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
      FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      const std::string& fsdbHost = "::1",
      int32_t fsdbPort = FLAGS_fsdbPort);
  void addStatePathSubscription(
      const std::vector<std::string>& subscribePath,
      FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
      FsdbStateSubscriber::FsdbOperStateUpdateCb operDeltaCb,
      const std::string& fsdbHost = "::1",
      int32_t fsdbPort = FLAGS_fsdbPort);
  void addStatDeltaSubscription(
      const std::vector<std::string>& subscribePath,
      FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
      FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
      const std::string& fsdbHost = "::1",
      int32_t fsdbPort = FLAGS_fsdbPort);
  void addStatPathSubscription(
      const std::vector<std::string>& subscribePath,
      FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
      FsdbStateSubscriber::FsdbOperStateUpdateCb operDeltaCb,
      const std::string& fsdbHost = "::1",
      int32_t fsdbPort = FLAGS_fsdbPort);

  /* Subscriber remove APIs */
  void removeStateDeltaSubscription(
      const std::vector<std::string>& subscribePath,
      const std::string& fsdbHost = "::1") {
    removeSubscriptionImpl(
        subscribePath, fsdbHost, true /*delta*/, false /*subscribeStats*/);
  }
  void removeStatePathSubscription(
      const std::vector<std::string>& subscribePath,
      const std::string& fsdbHost = "::1") {
    removeSubscriptionImpl(
        subscribePath, fsdbHost, false /*delta*/, false /*subscribeStats*/);
  }
  void removeStatDeltaSubscription(
      const std::vector<std::string>& subscribePath,
      const std::string& fsdbHost = "::1") {
    removeSubscriptionImpl(
        subscribePath, fsdbHost, true /*delta*/, true /*subscribeStats*/);
  }
  void removeStatPathSubscription(
      const std::vector<std::string>& subscribePath,
      const std::string& fsdbHost = "::1") {
    removeSubscriptionImpl(
        subscribePath, fsdbHost, false /*delta*/, true /*subscribeStats*/);
  }
  size_t numSubscriptions() const {
    return path2Subscriber_.rlock()->size();
  }

 private:
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
  void removeSubscriptionImpl(
      const std::vector<std::string>& subscribePath,
      const std::string& fsdbHost,
      bool isDelta,
      bool subscribeStats);
  template <typename SubscriberT>
  void addSubscriptionImpl(
      const std::vector<std::string>& subscribePath,
      FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
      typename SubscriberT::FsdbSubUnitUpdateCb subUnitAvailableCb,
      bool subscribeStats,
      const std::string& fsdbHost,
      int32_t fsdbPort = FLAGS_fsdbPort);

  folly::ScopedEventBaseThread reconnectThread_{"FsdbReconnectThread"};
  folly::ScopedEventBaseThread statsPublisherStreamEvbThread_{
      "FsdbStatsPublisherThread"};
  folly::ScopedEventBaseThread statePublisherStreamEvbThread_{
      "FsdbStatePublisherThread"};
  folly::ScopedEventBaseThread subscribersStreamEvbThread_{
      "FSdbSubscriberThread"};
  const std::string clientId_;
  std::mutex publisherMutex_;
  // State Publishers
  std::unique_ptr<FsdbDeltaPublisher> stateDeltaPublisher_;
  std::unique_ptr<FsdbStatePublisher> statePathPublisher_;
  // Stat Publishers
  std::unique_ptr<FsdbDeltaPublisher> statDeltaPublisher_;
  std::unique_ptr<FsdbStatePublisher> statPathPublisher_;
  folly::Synchronized<
      std::unordered_map<std::string, std::unique_ptr<FsdbStreamClient>>>
      path2Subscriber_;
};
} // namespace facebook::fboss::fsdb
