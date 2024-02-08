// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/server/ServiceHandler.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

#include <fboss/lib/LogThriftCall.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Timeout.h>
#include <folly/logging/xlog.h>
#include <range/v3/view.hpp>
#include "common/time/ChronoFlags.h"
#include "common/time/Time.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_constants.h"
#include "fboss/fsdb/oper/PathValidator.h"
#include "folly/CancellationToken.h"

#include <algorithm>
#include <iterator>

using namespace std::chrono_literals; // @donotremove
using namespace ranges;

DEFINE_time_s(metricsTtl, 1s * 12 * 3600 /* twelve hours */, "TTL for metrics");

DEFINE_bool(
    checkOperOwnership,
    false,
    "Enable/Disable permission checks on publisher id");

DEFINE_bool(trackMetadata, true, "Enable metadata tracking");

DEFINE_time_s(
    statsSubscriptionServe,
    10s,
    "Interval at which stats subscriptions are served");

DEFINE_time_s(
    statsSubscriptionHeartbeat,
    30s,
    "Interval at which heartbeats are sent for stats subscribers");

DEFINE_time_ms(
    stateSubscriptionServe,
    50ms,
    "Interval at which state subscriptions are served");

DEFINE_time_s(
    stateSubscriptionHeartbeat,
    5s,
    "Interval at which heartbeats are sent for state subscribers");

DEFINE_bool(
    enableOperDB,
    false,
    "Enable writing oper state changes to rocksdb");

DEFINE_bool(
    checkSubscriberConfig,
    true,
    "whether to check SubscriberConfig on subscription requests");

DEFINE_bool(
    enforceSubscriberConfig,
    false,
    "whether to enforce SubscriberConfig on subscription requests");

DEFINE_bool(
    enforcePublisherConfig,
    false,
    "whether to enforce PublisherConfig for publish stream requests");

static constexpr auto kWatchdogThreadHeartbeatMissed =
    "watchdog_thread_heartbeat_missed";

namespace {

using facebook::fboss::fsdb::OperPubRequest;
using facebook::fboss::fsdb::OperSubRequest;

template <typename OperRequest>
std::string getPubSubRequestDetails(const OperRequest& request) {
  static_assert(
      std::is_same_v<OperRequest, OperPubRequest> ||
      std::is_same_v<OperRequest, OperSubRequest>);
  std::string clientID = "";
  if constexpr (std::is_same_v<OperRequest, OperPubRequest>) {
    clientID = request.get_publisherId();
  } else if constexpr (std::is_same_v<OperRequest, OperSubRequest>) {
    clientID = request.get_subscriberId();
  }
  return fmt::format(
      "Client ID: {}, Path: {}",
      clientID,
      folly::join("/", request.path()->get_raw()));
}

} // namespace

namespace facebook::fboss::fsdb {

ServiceHandler::ServiceHandler(
    std::unique_ptr<FsdbConfig> fsdbConfig,
    const std::string& publisherIdsToOpenRocksDbAtStartFor,
    Options options)
    : FacebookBase2("FsdbService"),
      fsdbConfig_(std::move(fsdbConfig)),
      options_(std::move(options)),
      num_instances_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_instances")),
      num_publishers_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_publishers")),
      num_subscribers_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_subscribers")),
      num_subscriptions_rejected_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_subscriptions_disallowed"),
          fb303::SUM,
          fb303::RATE),
      num_publisher_unknown_requests_rejected_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_publisher_unknown_requests_rejected"),
          fb303::SUM,
          fb303::RATE),
      num_publisher_path_requests_rejected_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_publisher_path_requests_rejected"),
          fb303::SUM,
          fb303::RATE),
      num_dropped_stats_changes_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_dropped_stats_changes"),
          fb303::SUM,
          fb303::RATE),
      num_dropped_state_changes_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_dropped_state_changes"),
          fb303::SUM,
          fb303::RATE),
      operStorage_(
          {},
          FLAGS_stateSubscriptionServe_ms,
          FLAGS_stateSubscriptionHeartbeat_s,
          FLAGS_trackMetadata,
          "fsdb",
          options.serveIdPathSubs),
      operDbWriter_(operStorage_),
      operStatsStorage_(
          {},
          FLAGS_statsSubscriptionServe_s,
          FLAGS_statsSubscriptionHeartbeat_s,
          FLAGS_trackMetadata,
          "fsdb",
          options.serveIdPathSubs) {
  std::vector<PublisherId> publisherIds;
  // find publisherIds specified
  folly::split(',', publisherIdsToOpenRocksDbAtStartFor, publisherIds);

  num_instances_.incrementValue(1);

  operStorage_.start();
  operStatsStorage_.start();
  tcData().setCounter(kWatchdogThreadHeartbeatMissed, 0);

  // Create a watchdog that will monitor operStorage_ and operStatsStorage_
  // increment the missed counter when there is no heartbeat on at least one
  // thread in the last FLAGS_storage_thread_heartbeat_ms * 10 time
  XLOG(DBG1) << "Starting fsdb ServiceHandler thread heartbeat watchdog";
  heartbeatWatchdog_ = std::make_unique<ThreadHeartbeatWatchdog>(
      std::chrono::milliseconds(FLAGS_storage_thread_heartbeat_ms * 10),
      [this]() {
        watchdogThreadHeartbeatMissedCount_ += 1;
        tcData().setCounter(
            kWatchdogThreadHeartbeatMissed,
            watchdogThreadHeartbeatMissedCount_);
      });
  heartbeatWatchdog_->startMonitoringHeartbeat(
      operStorage_.getThreadHeartbeat());
  heartbeatWatchdog_->startMonitoringHeartbeat(
      operStatsStorage_.getThreadHeartbeat());
  heartbeatWatchdog_->start();

  if (FLAGS_enableOperDB) {
    rocksDbs_ = options_.useFakeRocksDb_CAUTION_DO_NOT_USE_IN_PRODUCTION
        ? createIfNeededAndOpenRocksDbs<RocksDbFake>(
              {publisherIds.begin(), publisherIds.end()})
        : createIfNeededAndOpenRocksDbs<RocksDb>(
              {publisherIds.begin(), publisherIds.end()});
    operDbWriter_.start();
  }
}

template <typename T>
folly::F14FastMap<PublisherId, std::shared_ptr<RocksDbIf>>
ServiceHandler::createIfNeededAndOpenRocksDbs(
    folly::F14FastSet<PublisherId> publisherIds) const {
  folly::F14FastMap<PublisherId, RocksDbPtr> ret;
  for (const auto& publisherId : publisherIds) {
    auto rocksDb = std::make_shared<T>(
        "stats",
        publisherId,
        FLAGS_metricsTtl_s,
        options_.eraseRocksDbsInCtorAndDtor_CAUTION_DO_NOT_USE_IN_PRODUCTION);
    const auto logPrefix = fmt::format("[P:{}]", publisherId);
    if (!rocksDb->open()) {
      throw Utils::createFsdbException(
          FsdbErrorCode::ROCKSDB_OPEN_OR_CREATE_FAILED,
          logPrefix,
          " could not open or create rocksdb");
    }
    XLOG(INFO) << logPrefix << " pre-created rocksdb";
    ret.insert({publisherId, rocksDb});
  }

  return ret;
}

ServiceHandler::~ServiceHandler() {
  if (heartbeatWatchdog_) {
    XLOG(DBG1) << "Stopping fsdb ServiceHandler thread heartbeat watchdog";
    heartbeatWatchdog_->stop();
    heartbeatWatchdog_.reset();
  }
  XLOG(INFO) << "Destroying ServiceHandler";
  num_instances_.incrementValue(-1);
}

OperPublisherInfo ServiceHandler::makePublisherInfo(
    const OperPubRequest& req,
    PubSubType type,
    bool isStats) {
  OperPublisherInfo info;
  info.publisherId() = *req.publisherId();
  info.type() = type;
  info.path() = *req.path();
  info.isStats() = isStats;
  try {
    auto pathConfig =
        fsdbConfig_->getPathConfig(*req.publisherId(), *req.path()->raw());
    info.isExpectedPath() = *pathConfig.get().isExpected();
  } catch (const std::exception& e) {
    // ignore exception if PathConfig is not available
  }
  return info;
}

void ServiceHandler::registerPublisher(const OperPublisherInfo& info) {
  if (info.publisherId()->empty()) {
    throw Utils::createFsdbException(
        FsdbErrorCode::EMPTY_PUBLISHER_ID, "Publisher Id must not be empty");
  }
  auto resp = activePublishers_.wlock()->insert(info);
  if (!resp.second) {
    throw Utils::createFsdbException(
        FsdbErrorCode::ID_ALREADY_EXISTS, "Dup publisher id");
  }
  if (*info.isStats()) {
    operStatsStorage_.registerPublisher(
        info.path()->raw()->begin(), info.path()->raw()->end());
  } else {
    operStorage_.registerPublisher(
        info.path()->raw()->begin(), info.path()->raw()->end());
  }
}

void ServiceHandler::unregisterPublisher(
    const OperPublisherInfo& info,
    FsdbErrorCode disconnectReason) {
  XLOG(DBG2) << " Publisher complete " << *info.publisherId() << " : "
             << folly::join("/", *info.path()->raw()) << " disconnectReason: "
             << apache::thrift::util::enumNameSafe(disconnectReason);
  activePublishers_.wlock()->erase(info);
  if (*info.isStats()) {
    operStatsStorage_.unregisterPublisher(
        info.path()->raw()->begin(),
        info.path()->raw()->end(),
        disconnectReason);
  } else {
    operStorage_.unregisterPublisher(
        info.path()->raw()->begin(),
        info.path()->raw()->end(),
        disconnectReason);
  }
}

template <typename PubUnit>
apache::thrift::SinkConsumer<PubUnit, OperPubFinalResponse>
ServiceHandler::makeSinkConsumer(
    std::unique_ptr<OperPubRequest> request,
    bool isStats) {
  validateOperPublishPermissions(
      *request->publisherId(), *request->path()->raw());
  PubSubType pubSubType{PubSubType::PATH};
  if constexpr (std::is_same_v<PubUnit, OperDelta>) {
    pubSubType = PubSubType::DELTA;
  }

  auto info = makePublisherInfo(*request, pubSubType, isStats);
  registerPublisher(info);
  std::shared_ptr<FsdbErrorCode> disconnectReason =
      std::make_shared<FsdbErrorCode>(FsdbErrorCode::ALL_PUBLISHERS_GONE);
  auto cleanupPublisher =
      folly::makeGuard([this, info = std::move(info), disconnectReason]() {
        unregisterPublisher(info, *disconnectReason);
      });

  apache::thrift::SinkConsumer<PubUnit, OperPubFinalResponse> consumer{
      [this,
       disconnectReason = std::move(disconnectReason),
       cleanupPublisher = std::move(cleanupPublisher),
       request = std::move(request),
       isStats](folly::coro::AsyncGenerator<PubUnit&&> gen)
          -> folly::coro::Task<OperPubFinalResponse> {
        OperPubFinalResponse finalResponse;
        try {
          while (auto chunk = co_await gen.next()) {
            XLOG(DBG3) << " chunk received";
            if (!chunk->metadata()) {
              chunk->metadata() = OperMetadata();
            }
            // Timestamp at server if chunk was not timestamped
            // by publisher
            if (!chunk->metadata()->lastConfirmedAt()) {
              chunk->metadata()->lastConfirmedAt() =
                  WallClockUtil::NowInSecFast();
            }
            if constexpr (std::is_same_v<PubUnit, OperState>) {
              if (isStats) {
                operStatsStorage_.set_encoded(
                    request->path()->raw()->begin(),
                    request->path()->raw()->end(),
                    *chunk);
              } else {
                operStorage_.set_encoded(
                    request->path()->raw()->begin(),
                    request->path()->raw()->end(),
                    *chunk);
              }
            } else {
              // filter out invalid paths for cases where publisher is newer
              // than fsdb
              auto isPathValid = isStats ? PathValidator::isStatsPathValid
                                         : PathValidator::isStatePathValid;
              auto numChanges = chunk->changes()->size();
              chunk->changes()->erase(
                  std::remove_if(
                      chunk->changes()->begin(),
                      chunk->changes()->end(),
                      [isPathValid](auto& change) {
                        return !isPathValid(*change.path()->raw());
                      }),
                  chunk->changes()->end());

              if (isStats) {
                operStatsStorage_.patch(*chunk);
              } else {
                operStorage_.patch(*chunk);
              }
              auto numDropped = numChanges - chunk->changes()->size();
              if (numDropped) {
                XLOG(DBG2) << "Dropping " << numDropped << " changes from "
                           << (isStats ? "stats" : "state")
                           << " chunk with invalid path";
                if (isStats) {
                  num_dropped_stats_changes_.addValue(numDropped);
                } else {
                  num_dropped_state_changes_.addValue(numDropped);
                }
              }
            }
          }
          co_return finalResponse;
        } catch (const fsdb::FsdbException& ex) {
          XLOG(ERR) << " Server:sink: FsdbException: "
                    << apache::thrift::util::enumNameSafe(ex.get_errorCode())
                    << ": " << ex.get_message();
          *disconnectReason = ex.get_errorCode();
          throw;
        } catch (const std::exception& e) {
          XLOG(INFO) << " Got exception in publish loop: " << e.what();
          throw;
        }
      },
      10 /* bufferSize */};
  return consumer;
}

folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
    OperPubInitResponse,
    OperState,
    OperPubFinalResponse>>
ServiceHandler::co_publishOperStatePath(
    std::unique_ptr<OperPubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getPubSubRequestDetails(*request));
  PathValidator::validateStatePath(*request->path()->raw());
  co_return {{}, makeSinkConsumer<OperState>(std::move(request), false)};
}

folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
    OperPubInitResponse,
    OperState,
    OperPubFinalResponse>>
ServiceHandler::co_publishOperStatsPath(
    std::unique_ptr<OperPubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getPubSubRequestDetails(*request));
  PathValidator::validateStatsPath(*request->path()->raw());
  co_return {{}, makeSinkConsumer<OperState>(std::move(request), true)};
}

folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
    OperPubInitResponse,
    OperDelta,
    OperPubFinalResponse>>
ServiceHandler::co_publishOperStateDelta(
    std::unique_ptr<OperPubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getPubSubRequestDetails(*request));
  PathValidator::validateStatePath(*request->path()->raw());
  co_return {{}, makeSinkConsumer<OperDelta>(std::move(request), false)};
}

folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
    OperPubInitResponse,
    OperDelta,
    OperPubFinalResponse>>
ServiceHandler::co_publishOperStatsDelta(
    std::unique_ptr<OperPubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getPubSubRequestDetails(*request));
  PathValidator::validateStatsPath(*request->path()->raw());
  co_return {{}, makeSinkConsumer<OperDelta>(std::move(request), true)};
}

namespace {

OperSubscriberInfo
makeSubscriberInfo(const OperSubRequest& req, PubSubType type, bool isStats) {
  OperSubscriberInfo info;
  info.subscriberId() = *req.subscriberId();
  info.type() = type;
  info.path() = *req.path();
  info.isStats() = isStats;
  info.subscribedSince() = static_cast<int64_t>(std::time(nullptr));
  return info;
}

OperSubscriberInfo makeSubscriberInfo(
    const OperSubRequestExtended& req,
    PubSubType type,
    bool isStats) {
  OperSubscriberInfo info;
  info.subscriberId() = *req.subscriberId();
  info.type() = type;
  info.extendedPaths() = *req.paths();
  info.isStats() = isStats;
  info.subscribedSince() = static_cast<int64_t>(std::time(nullptr));
  return info;
}

} // namespace

void ServiceHandler::registerSubscription(const OperSubscriberInfo& info) {
  if (info.subscriberId()->empty()) {
    throw Utils::createFsdbException(
        FsdbErrorCode::EMPTY_SUBSCRIBER_ID, "Subscriber Id must not be empty");
  }
  bool hasRawPath = info.path() && !info.path()->raw()->empty();
  bool hasExtendedPath = info.extendedPaths() && !info.extendedPaths()->empty();
  validateSubscriptionPermissions(
      *info.subscriberId(),
      *info.type(),
      *info.isStats(),
      hasRawPath,
      hasExtendedPath);
  auto resp = activeSubscriptions_.wlock()->insert(info);
  if (!resp.second) {
    throw Utils::createFsdbException(
        FsdbErrorCode::ID_ALREADY_EXISTS,
        "Dup subscriber id: ",
        *info.subscriberId());
  }
}
void ServiceHandler::unregisterSubscription(const OperSubscriberInfo& info) {
  XLOG(DBG2) << " Subscription complete " << *info.subscriberId() << " : "
             << folly::join("/", *info.path()->raw());
  activeSubscriptions_.wlock()->erase(info);
}

folly::coro::AsyncGenerator<DeltaValue<OperState>&&>
ServiceHandler::makeStateStreamGenerator(
    std::unique_ptr<OperSubRequest> request,
    bool isStats) {
  return isStats ? operStatsStorage_.subscribe_encoded(
                       *request->subscriberId(),
                       request->path()->raw()->begin(),
                       request->path()->raw()->end(),
                       *request->protocol())
                 : operStorage_.subscribe_encoded(
                       *request->subscriberId(),
                       request->path()->raw()->begin(),
                       request->path()->raw()->end(),
                       *request->protocol());
}

folly::coro::AsyncGenerator<std::vector<DeltaValue<TaggedOperState>>&&>
ServiceHandler::makeExtendedStateStreamGenerator(
    std::unique_ptr<OperSubRequestExtended> request,
    bool isStats) {
  return isStats ? operStatsStorage_.subscribe_encoded_extended(
                       *request->subscriberId(),
                       std::move(*request->paths()),
                       *request->protocol())
                 : operStorage_.subscribe_encoded_extended(
                       *request->subscriberId(),
                       std::move(*request->paths()),
                       *request->protocol());
}

folly::coro::Task<
    apache::thrift::ResponseAndServerStream<OperSubInitResponse, OperState>>
ServiceHandler::co_subscribeOperStatePath(
    std::unique_ptr<OperSubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getPubSubRequestDetails(*request));
  PathValidator::validateStatePath(*request->path()->raw());

  auto subscriberInfo = makeSubscriberInfo(*request, PubSubType::PATH, false);
  registerSubscription(subscriberInfo);
  auto cleanupSubscriber =
      folly::makeGuard([this, subscriberInfo = std::move(subscriberInfo)]() {
        unregisterSubscription(subscriberInfo);
      });

  co_return {
      {},
      folly::coro::co_invoke(
          [this,
           request = std::move(request),
           cleanupSubscriber = std::move(cleanupSubscriber)]() mutable
          -> folly::coro::AsyncGenerator<OperState&&> {
            auto generator =
                makeStateStreamGenerator(std::move(request), false);
            while (auto item = co_await generator.next()) {
              // got value
              auto&& delta = *item;
              if (delta.newVal) {
                co_yield std::move(*delta.newVal);
              } else {
                // yield empty OperState path
                OperState empty;
                empty.protocol() = OperProtocol::BINARY;
                co_yield std::move(empty);
              }
            }
          })};
}

folly::coro::Task<
    apache::thrift::ResponseAndServerStream<OperSubInitResponse, OperState>>
ServiceHandler::co_subscribeOperStatsPath(
    std::unique_ptr<OperSubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getPubSubRequestDetails(*request));
  PathValidator::validateStatsPath(*request->path()->raw());

  auto subscriberInfo = makeSubscriberInfo(*request, PubSubType::PATH, true);
  registerSubscription(subscriberInfo);
  auto cleanupSubscriber =
      folly::makeGuard([this, subscriberInfo = std::move(subscriberInfo)]() {
        unregisterSubscription(subscriberInfo);
      });

  co_return {
      {},
      folly::coro::co_invoke(
          [this,
           request = std::move(request),
           cleanupSubscriber = std::move(cleanupSubscriber)]() mutable
          -> folly::coro::AsyncGenerator<OperState&&> {
            auto generator = makeStateStreamGenerator(std::move(request), true);
            while (auto item = co_await generator.next()) {
              // got value
              auto&& delta = *item;
              if (delta.newVal) {
                co_yield std::move(*delta.newVal);
              } else {
                // yield empty OperState path
                OperState empty;
                empty.protocol() = OperProtocol::BINARY;
                co_yield std::move(empty);
              }
            }
          })};
}

folly::coro::AsyncGenerator<OperDelta&&>
ServiceHandler::makeDeltaStreamGenerator(
    std::unique_ptr<OperSubRequest> request,
    bool isStats) {
  return isStats ? operStatsStorage_.subscribe_delta(
                       *request->subscriberId(),
                       request->path()->raw()->begin(),
                       request->path()->raw()->end(),
                       *request->protocol())
                 : operStorage_.subscribe_delta(
                       *request->subscriberId(),
                       request->path()->raw()->begin(),
                       request->path()->raw()->end(),
                       *request->protocol());
}

folly::coro::AsyncGenerator<std::vector<TaggedOperDelta>&&>
ServiceHandler::makeExtendedDeltaStreamGenerator(
    std::unique_ptr<OperSubRequestExtended> request,
    bool isStats) {
  return isStats
      ? operStatsStorage_.subscribe_delta_extended(
            *request->subscriberId(), *request->paths(), *request->protocol())
      : operStorage_.subscribe_delta_extended(
            *request->subscriberId(), *request->paths(), *request->protocol());
}

folly::coro::Task<
    apache::thrift::ResponseAndServerStream<OperSubInitResponse, OperDelta>>
ServiceHandler::co_subscribeOperStateDelta(
    std::unique_ptr<OperSubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getPubSubRequestDetails(*request));
  PathValidator::validateStatePath(*request->path()->raw());

  auto subscriberInfo = makeSubscriberInfo(*request, PubSubType::DELTA, false);
  registerSubscription(subscriberInfo);
  auto cleanupSubscriber =
      folly::makeGuard([this, subscriberInfo = std::move(subscriberInfo)]() {
        unregisterSubscription(subscriberInfo);
      });

  co_return {
      {},
      folly::coro::co_invoke(
          [this,
           request = std::move(request),
           cleanupSubscriber = std::move(cleanupSubscriber)]() mutable
          -> folly::coro::AsyncGenerator<OperDelta&&> {
            auto generator =
                makeDeltaStreamGenerator(std::move(request), false);
            while (auto item = co_await generator.next()) {
              // got value
              co_yield std::move(*item);
            }
          })};
}

folly::coro::Task<apache::thrift::ResponseAndServerStream<
    OperSubInitResponse,
    OperSubPathUnit>>
ServiceHandler::co_subscribeOperStatePathExtended(
    std::unique_ptr<OperSubRequestExtended> request) {
  auto log = LOG_THRIFT_CALL(INFO);

  PathValidator::validateExtendedStatePaths(*request->paths());

  auto subscriberInfo = makeSubscriberInfo(*request, PubSubType::PATH, false);
  registerSubscription(subscriberInfo);
  auto cleanupSubscriber =
      folly::makeGuard([this, subscriberInfo = std::move(subscriberInfo)]() {
        unregisterSubscription(subscriberInfo);
      });

  co_return {
      {},
      folly::coro::co_invoke(
          [this,
           request = std::move(request),
           cleanupSubscriber = std::move(cleanupSubscriber)]() mutable
          -> folly::coro::AsyncGenerator<OperSubPathUnit&&> {
            auto generator =
                makeExtendedStateStreamGenerator(std::move(request), false);
            while (auto item = co_await generator.next()) {
              // got item
              auto&& deltas = *item;

              OperSubPathUnit unit;

              unit.changes() = deltas | view::move |
                  view::transform([](auto&& delta) {
                                 // we expect newVal to always be set, even in
                                 // the case of a deleted path. For deleted
                                 // paths, lower layers will create a
                                 // TaggedOperState with empty contents.
                                 return *std::move(delta.newVal);
                               }) |
                  to<std::vector>;

              co_yield std::move(unit);
            }
          })};
}

folly::coro::Task<apache::thrift::ResponseAndServerStream<
    OperSubInitResponse,
    OperSubDeltaUnit>>
ServiceHandler::co_subscribeOperStateDeltaExtended(
    std::unique_ptr<OperSubRequestExtended> request) {
  auto log = LOG_THRIFT_CALL(INFO);

  PathValidator::validateExtendedStatePaths(*request->paths());

  auto subscriberInfo = makeSubscriberInfo(*request, PubSubType::DELTA, false);
  registerSubscription(subscriberInfo);
  auto cleanupSubscriber =
      folly::makeGuard([this, subscriberInfo = std::move(subscriberInfo)]() {
        unregisterSubscription(subscriberInfo);
      });

  co_return {
      {},
      folly::coro::co_invoke(
          [this,
           request = std::move(request),
           cleanupSubscriber = std::move(cleanupSubscriber)]() mutable
          -> folly::coro::AsyncGenerator<OperSubDeltaUnit&&> {
            auto generator =
                makeExtendedDeltaStreamGenerator(std::move(request), false);
            while (auto item = co_await generator.next()) {
              // got item
              auto&& delta = *item;

              OperSubDeltaUnit unit;

              unit.changes() = delta;

              co_yield std::move(unit);
            }
          })};
}

folly::coro::Task<
    apache::thrift::ResponseAndServerStream<OperSubInitResponse, OperDelta>>
ServiceHandler::co_subscribeOperStatsDelta(
    std::unique_ptr<OperSubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getPubSubRequestDetails(*request));
  PathValidator::validateStatsPath(*request->path()->raw());

  auto subscriberInfo = makeSubscriberInfo(*request, PubSubType::DELTA, true);
  registerSubscription(subscriberInfo);
  auto cleanupSubscriber =
      folly::makeGuard([this, subscriberInfo = std::move(subscriberInfo)]() {
        unregisterSubscription(subscriberInfo);
      });

  co_return {
      {},
      folly::coro::co_invoke(
          [this,
           request = std::move(request),
           cleanupSubscriber = std::move(cleanupSubscriber)]() mutable
          -> folly::coro::AsyncGenerator<OperDelta&&> {
            auto generator = makeDeltaStreamGenerator(std::move(request), true);
            while (auto item = co_await generator.next()) {
              // got value
              co_yield std::move(*item);
            }
          })};
}

folly::coro::Task<apache::thrift::ResponseAndServerStream<
    OperSubInitResponse,
    OperSubPathUnit>>
ServiceHandler::co_subscribeOperStatsPathExtended(
    std::unique_ptr<OperSubRequestExtended> request) {
  auto log = LOG_THRIFT_CALL(INFO);

  PathValidator::validateExtendedStatsPaths(*request->paths());

  auto subscriberInfo = makeSubscriberInfo(*request, PubSubType::PATH, true);
  registerSubscription(subscriberInfo);
  auto cleanupSubscriber =
      folly::makeGuard([this, subscriberInfo = std::move(subscriberInfo)]() {
        unregisterSubscription(subscriberInfo);
      });

  co_return {
      {},
      folly::coro::co_invoke(
          [this,
           request = std::move(request),
           cleanupSubscriber = std::move(cleanupSubscriber)]() mutable
          -> folly::coro::AsyncGenerator<OperSubPathUnit&&> {
            auto generator =
                makeExtendedStateStreamGenerator(std::move(request), true);
            while (auto item = co_await generator.next()) {
              // got item
              auto&& deltas = *item;

              OperSubPathUnit unit;

              unit.changes() = deltas | view::move |
                  view::transform([](auto&& delta) {
                                 // we expect newVal to always be set, even in
                                 // the case of a deleted path. For deleted
                                 // paths, lower layers will create a
                                 // TaggedOperState with empty contents.
                                 return *std::move(delta.newVal);
                               }) |
                  to<std::vector>;

              co_yield std::move(unit);
            }
          })};
}

folly::coro::Task<apache::thrift::ResponseAndServerStream<
    OperSubInitResponse,
    OperSubDeltaUnit>>
ServiceHandler::co_subscribeOperStatsDeltaExtended(
    std::unique_ptr<OperSubRequestExtended> request) {
  auto log = LOG_THRIFT_CALL(INFO);

  PathValidator::validateExtendedStatsPaths(*request->paths());

  auto subscriberInfo = makeSubscriberInfo(*request, PubSubType::DELTA, true);
  registerSubscription(subscriberInfo);
  auto cleanupSubscriber =
      folly::makeGuard([this, subscriberInfo = std::move(subscriberInfo)]() {
        unregisterSubscription(subscriberInfo);
      });

  co_return {
      {},
      folly::coro::co_invoke(
          [this,
           request = std::move(request),
           cleanupSubscriber = std::move(cleanupSubscriber)]() mutable
          -> folly::coro::AsyncGenerator<OperSubDeltaUnit&&> {
            auto generator =
                makeExtendedDeltaStreamGenerator(std::move(request), true);
            while (auto item = co_await generator.next()) {
              // got item
              auto&& delta = *item;

              OperSubDeltaUnit unit;

              unit.changes() = delta;

              co_yield std::move(unit);
            }
          })};
}

folly::coro::Task<std::unique_ptr<OperState>> ServiceHandler::co_getOperState(
    std::unique_ptr<OperGetRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO);
  PathValidator::validateStatePath(*request->path()->raw());
  auto ret =
      std::make_unique<OperState>(operStorage_
                                      .get_encoded(
                                          request->path()->raw()->begin(),
                                          request->path()->raw()->end(),
                                          *request->protocol())
                                      .value());
  co_return std::move(ret);
}

folly::coro::Task<std::unique_ptr<OperState>> ServiceHandler::co_getOperStats(
    std::unique_ptr<OperGetRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO);
  PathValidator::validateStatsPath(*request->path()->raw());
  auto ret =
      std::make_unique<OperState>(operStatsStorage_
                                      .get_encoded(
                                          request->path()->raw()->begin(),
                                          request->path()->raw()->end(),
                                          *request->protocol())
                                      .value());
  co_return std::move(ret);
}

folly::coro::Task<std::unique_ptr<std::vector<TaggedOperState>>>
ServiceHandler::co_getOperStateExtended(
    std::unique_ptr<OperGetRequestExtended> request) {
  auto log = LOG_THRIFT_CALL(INFO);
  PathValidator::validateExtendedStatePaths(*request->paths());

  auto ret = std::make_unique<std::vector<TaggedOperState>>();

  for (const auto& path : *request->paths()) {
    auto curr = operStorage_.get_encoded_extended(
        path.path()->begin(), path.path()->end(), *request->protocol());
    ret->insert(
        ret->end(),
        std::make_move_iterator(curr->begin()),
        std::make_move_iterator(curr->end()));
  }

  co_return std::move(ret);
}

folly::coro::Task<std::unique_ptr<std::vector<TaggedOperState>>>
ServiceHandler::co_getOperStatsExtended(
    std::unique_ptr<OperGetRequestExtended> request) {
  auto log = LOG_THRIFT_CALL(INFO);
  PathValidator::validateExtendedStatsPaths(*request->paths());

  auto ret = std::make_unique<std::vector<TaggedOperState>>();

  for (const auto& path : *request->paths()) {
    auto curr = operStatsStorage_.get_encoded_extended(
        path.path()->begin(), path.path()->end(), *request->protocol());
    ret->insert(
        ret->end(),
        std::make_move_iterator(curr->begin()),
        std::make_move_iterator(curr->end()));
  }

  co_return std::move(ret);
}

// --------------------------------------------

std::shared_ptr<RocksDbIf> ServiceHandler::getRocksDb(
    const PublisherId& publisherId) const {
  const auto logPrefix = fmt::format("[P:{}]", publisherId);
  XLOG(INFO) << logPrefix << " find opened rocksdb";
  auto it = rocksDbs_.find(publisherId);
  if (it == rocksDbs_.end()) {
    throw Utils::createFsdbException(
        FsdbErrorCode::UNKNOWN_PUBLISHER,
        logPrefix,
        " FSDB does not have rocksdb instance opened - include publisher in gflags to fix.");
  }
  return it->second;
}

// --------------------------------------------

folly::coro::Task<std::unique_ptr<PublisherIdToOperPublisherInfo>>
ServiceHandler::co_getAllOperPublisherInfos() {
  auto publishers = std::make_unique<PublisherIdToOperPublisherInfo>();
  activePublishers_.withRLock([&](const auto& activePublishers) {
    for (const auto& publisher : activePublishers) {
      (*publishers)[*publisher.publisherId()].push_back(publisher);
    }
  });
  co_return publishers;
}

folly::coro::Task<std::unique_ptr<PublisherIdToOperPublisherInfo>>
ServiceHandler::co_getOperPublisherInfos(
    std::unique_ptr<PublisherIds> publisherIds) {
  auto log = LOG_THRIFT_CALL(INFO);
  auto publishers = std::make_unique<PublisherIdToOperPublisherInfo>();
  activePublishers_.withRLock([&](const auto& activePublishers) {
    for (auto& publisher : activePublishers) {
      if (publisherIds->find(*publisher.publisherId()) == publisherIds->end()) {
        continue;
      }
      (*publishers)[*publisher.publisherId()].push_back(publisher);
    }
  });
  co_return publishers;
}

folly::coro::Task<std::unique_ptr<SubscriberIdToOperSubscriberInfos>>
ServiceHandler::co_getAllOperSubscriberInfos() {
  auto subscriptions = std::make_unique<SubscriberIdToOperSubscriberInfos>();
  activeSubscriptions_.withRLock([&](const auto& activeSubscriptions) {
    for (const auto& subscription : activeSubscriptions) {
      (*subscriptions)[*subscription.subscriberId()].push_back(subscription);
    }
  });
  co_return subscriptions;
}

folly::coro::Task<std::unique_ptr<SubscriberIdToOperSubscriberInfos>>
ServiceHandler::co_getOperSubscriberInfos(
    std::unique_ptr<SubscriberIds> subscriberIds) {
  auto log = LOG_THRIFT_CALL(INFO);
  auto subscriptions = std::make_unique<SubscriberIdToOperSubscriberInfos>();
  activeSubscriptions_.withRLock([&](const auto& activeSubscriptions) {
    for (auto& subscription : activeSubscriptions) {
      if (subscriberIds->find(*subscription.subscriberId()) ==
          subscriberIds->end()) {
        continue;
      }
      (*subscriptions)[*subscription.subscriberId()].push_back(subscription);
    }
  });
  co_return subscriptions;
}

void ServiceHandler::validateSubscriptionPermissions(
    SubscriberId id,
    PubSubType /* type */,
    bool /* isStats */,
    bool hasRawPath,
    bool hasExtendedPath) {
  if (!hasRawPath && !hasExtendedPath) {
    // No subscriptions allowed at root. Subscription must be contained within
    // a publisher root.
    throw Utils::createFsdbException(
        FsdbErrorCode::INVALID_PATH, "Subscription path cannot be empty");
  }

  if (!FLAGS_checkSubscriberConfig) {
    return;
  }
  auto config = fsdbConfig_->getSubscriberConfig(id);
  if (hasExtendedPath && config.has_value() &&
      !*config.value().get().allowExtendedSubscriptions()) {
    XLOG(WARNING) << "[S:" << id << "]: extended subscriptions not permitted";
    num_subscriptions_rejected_.addValue(1);
    if (FLAGS_enforceSubscriberConfig) {
      throw Utils::createFsdbException(
          FsdbErrorCode::SUBSCRIPTION_NOT_PERMITTED,
          fmt::format(
              "[S:{}]: extended subscriptions not permitted", std::string(id)));
    }
  }
}

void ServiceHandler::validateOperPublishPermissions(
    PublisherId id,
    const std::vector<std::string>& path) {
  if (!FLAGS_checkOperOwnership) {
    return;
  }

  try {
    fsdbConfig_->getPathConfig(id, path);
    // path is configured, so we have permission to publish
    return;
  } catch (const fsdb::FsdbException& ex) {
    if (ex.errorCode() == FsdbErrorCode::PUBLISHER_NOT_PERMITTED) {
      num_publisher_path_requests_rejected_.addValue(1);
    } else {
      num_publisher_unknown_requests_rejected_.addValue(1);
    }
    if (FLAGS_enforcePublisherConfig) {
      throw;
    }
    return;
  }
}

} // namespace facebook::fboss::fsdb
