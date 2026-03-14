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
#include "fboss/fsdb/oper/instantiations/FsdbNaivePeriodicSubscribableStorage.h"
#include "fboss/fsdb/server/FsdbConfig.h"
#include "fboss/fsdb/server/FsdbOperTreeMetadataTracker.h"
#include "fboss/lib/ThreadHeartbeat.h"
#include "re2/re2.h"

DECLARE_int32(statsSubscriptionServeQueueSize);
DECLARE_int32(deltaSubscriptionQueueFullMinSize);
DECLARE_int32(deltaSubscriptionQueueMemoryLimit_mb);
DECLARE_int32(statsSubscriptionHeartbeat_s);
DECLARE_int32(stateSubscriptionHeartbeat_s);

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

    bool serveIdPathSubs{false};

    Options&& setServeIdPathSubs(bool val) && {
      serveIdPathSubs = val;
      return std::move(*this);
    }
  };

  using RawPathT = std::vector<std::string>;

  ServiceHandler(
      std::shared_ptr<FsdbConfig> fsdbConfig,
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

  // Patch apis
  folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
      OperPubInitResponse,
      PublisherMessage,
      OperPubFinalResponse>>
  co_publishState(std::unique_ptr<PubRequest> request) override;

  folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
      OperPubInitResponse,
      PublisherMessage,
      OperPubFinalResponse>>
  co_publishStats(std::unique_ptr<PubRequest> request) override;

  folly::coro::Task<apache::thrift::ResponseAndServerStream<
      OperSubInitResponse,
      SubscriberMessage>>
  co_subscribeState(std::unique_ptr<SubRequest> request) override;

  folly::coro::Task<apache::thrift::ResponseAndServerStream<
      OperSubInitResponse,
      SubscriberMessage>>
  co_subscribeStats(std::unique_ptr<SubRequest> request) override;

  folly::coro::Task<apache::thrift::ResponseAndServerStream<
      OperSubInitResponse,
      SubscriberMessage>>
  co_subscribeStateExtended(std::unique_ptr<SubRequest> request) override;

  folly::coro::Task<apache::thrift::ResponseAndServerStream<
      OperSubInitResponse,
      SubscriberMessage>>
  co_subscribeStatsExtended(std::unique_ptr<SubRequest> request) override;

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

  // Client key (clientId, Path, PubSubType, isStats)
  using ClientKey = std::tuple<std::string, Path, PubSubType, bool>;
  using ActiveSubscriptions =
      std::map<ClientKey, std::vector<OperSubscriberInfo>>;
  using ActivePublishers = std::map<ClientKey, OperPublisherInfo>;

  ActiveSubscriptions getActiveSubscriptions() const {
    return activeSubscriptions_.copy();
  }
  ActivePublishers getActivePublishers() const {
    return activePublishers_.copy();
  }

  void setThriftServer(std::shared_ptr<apache::thrift::ThriftServer> server) {
    server_ = server;
  }

  void preStart(const folly::SocketAddress* /*address*/) override;

 private:
  SubscriptionIdentifier makeSubscriptionIdentifier(
      const OperSubscriberInfo& info);
  void registerSubscription(
      const OperSubscriberInfo& info,
      bool forceSubscribe = false);
  void unregisterSubscription(const OperSubscriberInfo& info);
  void updateSubscriptionCounters(
      const OperSubscriberInfo& info,
      bool isConnected,
      bool uniqueSubForClient);
  void registerPublisher(const OperPublisherInfo& info);
  void unregisterPublisher(
      const OperPublisherInfo& info,
      FsdbErrorCode disconnectReason);
  template <typename PubUnit>
  apache::thrift::SinkConsumer<PubUnit, OperPubFinalResponse> makeSinkConsumer(
      RawPathT&& path,
      const PublisherId& publisherId,
      bool isStats);

  folly::coro::AsyncGenerator<DeltaValue<OperState>&&> makeStateStreamGenerator(
      std::unique_ptr<OperSubRequest> request,
      bool isStats,
      SubscriptionIdentifier&& subId);

  SubscriptionStreamReader<SubscriptionServeQueueElement<OperDelta>>
  makeDeltaStreamGenerator(
      std::unique_ptr<OperSubRequest> request,
      bool isStats,
      SubscriptionIdentifier&& subId);

  folly::coro::AsyncGenerator<std::vector<DeltaValue<TaggedOperState>>&&>
  makeExtendedStateStreamGenerator(
      std::unique_ptr<OperSubRequestExtended> request,
      bool isStats,
      SubscriptionIdentifier&& subId);

  SubscriptionStreamReader<SubscriptionServeQueueElement<SubscriberMessage>>
  makePatchStreamGenerator(
      std::unique_ptr<SubRequest> request,
      bool isStats,
      SubscriptionIdentifier&& subId);

  SubscriptionStreamReader<
      SubscriptionServeQueueElement<std::vector<TaggedOperDelta>>>
  makeExtendedDeltaStreamGenerator(
      std::unique_ptr<OperSubRequestExtended> request,
      bool isStats,
      SubscriptionIdentifier&& subId);

  OperPublisherInfo makePublisherInfo(
      const RawPathT& path,
      const PublisherId& publisherId,
      PubSubType type,
      bool isStats);

  void initFlagDefaults(
      const std::unordered_map<std::string, std::string>& flags);

  void initPerStreamCounters();

  void validateSubscriptionPermissions(
      SubscriberId id,
      PubSubType type,
      bool isStats,
      bool hasRawPath,
      bool hasExtendedPath);

  void validateOperPublishPermissions(PublisherId id, const RawPathT& path);

  std::shared_ptr<FsdbConfig> fsdbConfig_;

  const Options options_;

  using TLCounter = fb303::ThreadCachedServiceData::TLCounter;
  using TLTimeseries = fb303::ThreadCachedServiceData::TLTimeseries;
  using PublisherKey = std::pair<PublisherId, bool>;
  TLCounter num_instances_;
  TLCounter num_publishers_;
  TLCounter num_subscribers_;
  TLCounter num_subscriptions_;
  TLCounter num_disconnected_subscribers_;
  TLCounter num_disconnected_subscriptions_;
  TLCounter num_disconnected_publishers_;
  std::map<SubscriberId, TLCounter> disconnectedSubscribers_;
  std::map<SubscriberId, TLCounter> connectedSubscriptions_;
  std::map<SubscriberId, TLCounter> disconnectedSubscriptions_;
  std::map<PublisherKey, TLCounter> disconnectedPublishers_;
  TLTimeseries num_subscriptions_rejected_;
  TLTimeseries num_publisher_unknown_requests_rejected_;
  TLTimeseries num_publisher_path_requests_rejected_;
  TLTimeseries num_dropped_stats_changes_;
  TLTimeseries num_dropped_state_changes_;
  FsdbNaivePeriodicSubscribableStorage operStorage_;
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

  folly::Synchronized<ActiveSubscriptions> activeSubscriptions_;
  folly::Synchronized<ActivePublishers> activePublishers_;
  std::shared_ptr<apache::thrift::ThriftServer> server_;
  std::atomic<uint64_t> lastSubscriptionUid_{1};
};

} // namespace facebook::fboss::fsdb
