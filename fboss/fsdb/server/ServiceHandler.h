// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <common/fb303/cpp/FacebookBase2.h>

#include <fb303/ThreadCachedServiceData.h>
#include <folly/Synchronized.h>
#include <folly/container/F14Map.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/lib/cpp/server/TServerEventHandler.h>
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/fsdb/oper/DbWriter.h"
#include "fboss/fsdb/oper/instantiations/FsdbNaivePeriodicSubscribableStorage.h"
#include "fboss/fsdb/server/FsdbConfig.h"
#include "fboss/fsdb/server/FsdbOperTreeMetadataTracker.h"
#include "fboss/fsdb/server/RocksDb.h"
#include "fboss/lib/ThreadHeartbeat.h"
#include "re2/re2.h"

DECLARE_bool(checkSubscriberConfig);
DECLARE_bool(enforceSubscriberConfig);
DECLARE_bool(checkOperOwnership);
DECLARE_bool(enforcePublisherConfig);

namespace facebook::fboss::fsdb {
class ServiceHandler : public FsdbServiceSvIf,
                       public facebook::fb303::FacebookBase2,
                       public apache::thrift::server::TServerEventHandler,
                       public folly::MoveOnly {
 public:
  struct Options {
    Options() {}

    /* variable names absurd by design */
    bool eraseRocksDbsInCtorAndDtor_CAUTION_DO_NOT_USE_IN_PRODUCTION{false};
    bool useFakeRocksDb_CAUTION_DO_NOT_USE_IN_PRODUCTION{false};
    bool serveIdPathSubs{false};

    Options&& setServeIdPathSubs(bool val) && {
      serveIdPathSubs = val;
      return std::move(*this);
    }
  };

  ServiceHandler(
      std::unique_ptr<FsdbConfig> fsdbConfig,
      const std::string& publisherIdsToOpenRocksDbAtStartFor,
      Options options = Options());
  ~ServiceHandler() override;

  facebook::fb303::cpp2::fb_status getStatus() override {
    return facebook::fb303::cpp2::fb_status::ALIVE;
  }

  // Oper related

  folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
      OperPubInitResponse,
      OperState,
      OperPubFinalResponse>>
  co_publishOperStatePath(std::unique_ptr<OperPubRequest> request) override;

  folly::coro::Task<
      apache::thrift::ResponseAndServerStream<OperSubInitResponse, OperState>>
      co_subscribeOperStatePath(
          std::unique_ptr<OperSubRequest> /*request*/) override;

  folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
      OperPubInitResponse,
      OperDelta,
      OperPubFinalResponse>>
  co_publishOperStateDelta(std::unique_ptr<OperPubRequest> request) override;

  folly::coro::Task<
      apache::thrift::ResponseAndServerStream<OperSubInitResponse, OperDelta>>
      co_subscribeOperStateDelta(
          std::unique_ptr<OperSubRequest> /*request*/) override;

  folly::coro::Task<apache::thrift::ResponseAndServerStream<
      OperSubInitResponse,
      OperSubPathUnit>>
      co_subscribeOperStatePathExtended(
          std::unique_ptr<OperSubRequestExtended> /*request*/) override;

  folly::coro::Task<apache::thrift::ResponseAndServerStream<
      OperSubInitResponse,
      OperSubDeltaUnit>>
      co_subscribeOperStateDeltaExtended(
          std::unique_ptr<OperSubRequestExtended> /*request*/) override;

  folly::coro::Task<std::unique_ptr<OperState>> co_getOperState(
      std::unique_ptr<OperGetRequest> request) override;
  folly::coro::Task<std::unique_ptr<OperState>> co_getOperStats(
      std::unique_ptr<OperGetRequest> request) override;

  folly::coro::Task<std::unique_ptr<std::vector<TaggedOperState>>>
  co_getOperStateExtended(
      std::unique_ptr<OperGetRequestExtended> request) override;
  folly::coro::Task<std::unique_ptr<std::vector<TaggedOperState>>>
  co_getOperStatsExtended(
      std::unique_ptr<OperGetRequestExtended> request) override;

  // Oper stats related
  folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
      OperPubInitResponse,
      OperState,
      OperPubFinalResponse>>
  co_publishOperStatsPath(std::unique_ptr<OperPubRequest> request) override;

  folly::coro::Task<
      apache::thrift::ResponseAndServerStream<OperSubInitResponse, OperState>>
      co_subscribeOperStatsPath(
          std::unique_ptr<OperSubRequest> /*request*/) override;

  folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
      OperPubInitResponse,
      OperDelta,
      OperPubFinalResponse>>
  co_publishOperStatsDelta(std::unique_ptr<OperPubRequest> request) override;

  folly::coro::Task<
      apache::thrift::ResponseAndServerStream<OperSubInitResponse, OperDelta>>
      co_subscribeOperStatsDelta(
          std::unique_ptr<OperSubRequest> /*request*/) override;

  folly::coro::Task<apache::thrift::ResponseAndServerStream<
      OperSubInitResponse,
      OperSubPathUnit>>
      co_subscribeOperStatsPathExtended(
          std::unique_ptr<OperSubRequestExtended> /*request*/) override;

  folly::coro::Task<apache::thrift::ResponseAndServerStream<
      OperSubInitResponse,
      OperSubDeltaUnit>>
      co_subscribeOperStatsDeltaExtended(
          std::unique_ptr<OperSubRequestExtended> /*request*/) override;

  // Management Plane related ---------------------------------------

  folly::coro::Task<std::unique_ptr<PublisherIdToOperPublisherInfo>>
  co_getAllOperPublisherInfos() override;

  folly::coro::Task<std::unique_ptr<PublisherIdToOperPublisherInfo>>
  co_getOperPublisherInfos(std::unique_ptr<PublisherIds> publisherIds) override;

  folly::coro::Task<std::unique_ptr<SubscriberIdToOperSubscriberInfos>>
  co_getAllOperSubscriberInfos() override;

  folly::coro::Task<std::unique_ptr<SubscriberIdToOperSubscriberInfos>>
  co_getOperSubscriberInfos(
      std::unique_ptr<SubscriberIds> subscriberIds) override;

  // Expensive API to copy root thrift object. To be used only
  // in tests.
  FsdbOperStateRoot operRootExpensive() const {
    return operStorage_.currentStateExpensive();
  }
  // Expensive API to copy root thrift object. To be used only
  // in tests.
  FsdbOperStatsRoot operStatsRootExpensive() const {
    return operStatsStorage_.currentStateExpensive();
  }

  FsdbOperTreeMetadataTracker getStatsPublisherMetadata() const {
    return operStatsStorage_.getMetadata();
  }
  FsdbOperTreeMetadataTracker getStatePublisherMetadata() const {
    return operStorage_.getMetadata();
  }

  std::set<OperSubscriberInfo> getActiveSubscriptions() const {
    return activeSubscriptions_.copy();
  }
  std::set<OperPublisherInfo> getActivePublishers() const {
    return activePublishers_.copy();
  }

 private:
  void registerSubscription(const OperSubscriberInfo& info);
  void unregisterSubscription(const OperSubscriberInfo& info);
  void registerPublisher(const OperPublisherInfo& info);
  void unregisterPublisher(
      const OperPublisherInfo& info,
      FsdbErrorCode disconnectReason);
  template <typename PubUnit>
  apache::thrift::SinkConsumer<PubUnit, OperPubFinalResponse> makeSinkConsumer(
      std::unique_ptr<OperPubRequest> request,
      bool isStats);

  folly::coro::AsyncGenerator<DeltaValue<OperState>&&> makeStateStreamGenerator(
      std::unique_ptr<OperSubRequest> request,
      bool isStats);

  folly::coro::AsyncGenerator<OperDelta&&> makeDeltaStreamGenerator(
      std::unique_ptr<OperSubRequest> request,
      bool isStats);

  folly::coro::AsyncGenerator<std::vector<DeltaValue<TaggedOperState>>&&>
  makeExtendedStateStreamGenerator(
      std::unique_ptr<OperSubRequestExtended> request,
      bool isStats);

  folly::coro::AsyncGenerator<std::vector<TaggedOperDelta>&&>
  makeExtendedDeltaStreamGenerator(
      std::unique_ptr<OperSubRequestExtended> request,
      bool isStats);

  OperPublisherInfo
  makePublisherInfo(const OperPubRequest& req, PubSubType type, bool isStats);

  void initFlagDefaults(
      const std::unordered_map<std::string, std::string>& flags);

  using RocksDbPtr = std::shared_ptr<RocksDbIf>;
  template <typename T>
  folly::F14FastMap<PublisherId, RocksDbPtr> createIfNeededAndOpenRocksDbs(
      folly::F14FastSet<PublisherId> publisherIds) const;
  RocksDbPtr getRocksDb(const PublisherId& publisherId) const;

  void validateSubscriptionPermissions(
      SubscriberId id,
      PubSubType type,
      bool isStats,
      bool hasRawPath,
      bool hasExtendedPath);

  void validateOperPublishPermissions(
      PublisherId id,
      const std::vector<std::string>& path);

  std::unique_ptr<FsdbConfig> fsdbConfig_;
  folly::F14FastMap<PublisherId, RocksDbPtr> rocksDbs_; // const after ctor

  const Options options_;

  using TLCounter = fb303::ThreadCachedServiceData::TLCounter;
  using TLTimeseries = fb303::ThreadCachedServiceData::TLTimeseries;
  TLCounter num_instances_;
  TLCounter num_publishers_;
  TLCounter num_subscribers_;
  TLTimeseries num_subscriptions_rejected_;
  TLTimeseries num_publisher_unknown_requests_rejected_;
  TLTimeseries num_publisher_path_requests_rejected_;
  TLTimeseries num_dropped_stats_changes_;
  TLTimeseries num_dropped_state_changes_;
  FsdbNaivePeriodicSubscribableStorage operStorage_;
  DbWriter operDbWriter_;
  // TODO - decide on right DB abstraction for stats
  FsdbNaivePeriodicSubscribableStatsStorage operStatsStorage_;

  /*
   * A thread dedicated to monitor operStorage_ and operStatsStorage_
   */
  std::unique_ptr<ThreadHeartbeatWatchdog> heartbeatWatchdog_;

  /*
   * Tracks how many times a heart beat was missed from monitered threads
   * This counter is periodically published to ODS
   */
  std::atomic<long> watchdogThreadHeartbeatMissedCount_{0};

  folly::Synchronized<std::set<OperSubscriberInfo>> activeSubscriptions_;
  folly::Synchronized<std::set<OperPublisherInfo>> activePublishers_;
};

} // namespace facebook::fboss::fsdb
