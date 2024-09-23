// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/server/ServiceHandler.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"

#include <fboss/lib/LogThriftCall.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Timeout.h>
#include <folly/logging/xlog.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_constants.h"
#include "fboss/fsdb/oper/PathValidator.h"
#include "folly/CancellationToken.h"

#include <algorithm>
#include <chrono>
#include <iterator>

using namespace std::chrono_literals; // @donotremove

DEFINE_int32(metricsTtl_s, 1 * 12 * 3600 /* twelve hours */, "TTL for metrics");

DEFINE_bool(
    checkOperOwnership,
    false,
    "Enable/Disable permission checks on publisher id");

DEFINE_bool(trackMetadata, true, "Enable metadata tracking");

DEFINE_int32(
    statsSubscriptionServe_s,
    10,
    "Interval at which stats subscriptions are served");

DEFINE_int32(
    statsSubscriptionHeartbeat_s,
    30,
    "Interval at which heartbeats are sent for stats subscribers");

DEFINE_int32(
    stateSubscriptionServe_ms,
    50,
    "Interval at which state subscriptions are served");

DEFINE_int32(
    stateSubscriptionHeartbeat_s,
    5,
    "Interval at which heartbeats are sent for state subscribers");

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
DEFINE_bool(
    forceRegisterSubscriptions,
    false,
    "Whether to bypass unique subscriber check. Should only be used during debugging");

static constexpr auto kWatchdogThreadHeartbeatMissed =
    "watchdog_thread_heartbeat_missed";

namespace {

using facebook::fboss::fsdb::ExtendedOperPath;
using facebook::fboss::fsdb::OperGetRequest;
using facebook::fboss::fsdb::OperPath;
using facebook::fboss::fsdb::OperPubRequest;
using facebook::fboss::fsdb::OperSubRequest;
using facebook::fboss::fsdb::OperSubRequestExtended;
using facebook::fboss::fsdb::Path;
using facebook::fboss::fsdb::PubRequest;
using facebook::fboss::fsdb::SubRequest;

template <typename OperRequest>
std::string getRequestDetails(const OperRequest& request) {
  static_assert(
      std::is_same_v<OperRequest, OperPubRequest> ||
      std::is_same_v<OperRequest, OperSubRequest> ||
      std::is_same_v<OperRequest, OperSubRequestExtended> ||
      std::is_same_v<OperRequest, OperGetRequest> ||
      std::is_same_v<OperRequest, SubRequest> ||
      std::is_same_v<OperRequest, PubRequest>);
  std::string clientID = "";
  if constexpr (
      std::is_same_v<OperRequest, PubRequest> ||
      std::is_same_v<OperRequest, SubRequest>) {
    clientID = *request.clientId()->instanceId();
  } else if constexpr (std::is_same_v<OperRequest, OperPubRequest>) {
    clientID = request.get_publisherId();
  } else if constexpr (std::is_same_v<OperRequest, OperSubRequest>) {
    clientID = request.get_subscriberId();
  } else if constexpr (std::is_same_v<OperRequest, OperSubRequestExtended>) {
    clientID = request.get_subscriberId();
  } else if constexpr (std::is_same_v<OperRequest, OperGetRequest>) {
    // TODO: enforce clientId on polling apis
    clientID = "adhoc";
  }
  std::string pathStr = "";
  if constexpr (
      std::is_same_v<OperRequest, OperPubRequest> ||
      std::is_same_v<OperRequest, OperSubRequest>) {
    pathStr = folly::join("/", request.path()->get_raw());
  } else if constexpr (std::is_same_v<OperRequest, PubRequest>) {
    pathStr = folly::join("/", request.path()->get_path());
  } else if constexpr (std::is_same_v<OperRequest, SubRequest>) {
    std::vector<std::string> pathStrings;
    pathStrings.reserve(request.paths()->size());
    for (const auto& path : *request.paths()) {
      pathStrings.push_back(fmt::format(
          "{}:{}", path.first, folly::join("/", *path.second.path())));
    }
    pathStr = folly::join(", ", std::move(pathStrings));
  } else if constexpr (std::is_same_v<OperRequest, OperSubRequestExtended>) {
    // TODO: set path str for extended subs
  }
  return fmt::format("Client ID: {}, Path: {}", std::move(clientID), pathStr);
}

Path buildPathUnion(facebook::fboss::fsdb::OperSubscriberInfo info) {
  Path pathUnion;
  if (info.path() && info.path()->raw()->size() > 0) {
    OperPath operPath;
    operPath.raw() = *info.path()->raw();
    pathUnion.set_operPath(operPath);
  } else if (info.extendedPaths() && info.extendedPaths()->size() > 0) {
    std::vector<ExtendedOperPath> extendedPaths;
    pathUnion.set_extendedPaths(*info.extendedPaths());
  } else if (info.paths() && info.paths()->size() > 0) {
    pathUnion.set_rawPaths(*info.paths());
  }
  return pathUnion;
}

Path buildPathUnion(facebook::fboss::fsdb::OperPublisherInfo info) {
  Path pathUnion;
  OperPath operPath;
  operPath.raw() = *info.path()->raw();
  pathUnion.set_operPath(operPath);
  return pathUnion;
}

void updateMetadata(facebook::fboss::fsdb::OperMetadata& metadata) {
  // Timestamp at server if chunk was not timestamped
  // by publisher
  if (!metadata.lastConfirmedAt()) {
    auto now = std::chrono::system_clock::now();
    metadata.lastConfirmedAt() =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();
  }
}

} // namespace

namespace facebook::fboss::fsdb {

ServiceHandler::ServiceHandler(
    std::shared_ptr<FsdbConfig> fsdbConfig,
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
      num_subscriptions_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_subscriptions")),
      num_disconnected_subscribers_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_disconnected_subscribers")),
      num_disconnected_subscriptions_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_disconnected_subscriptions")),
      num_disconnected_publishers_(
          fb303::ThreadCachedServiceData::get()->getThreadStats(),
          folly::to<std::string>(
              fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
              "num_disconnected_publishers")),
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
          std::chrono::milliseconds(FLAGS_stateSubscriptionServe_ms),
          std::chrono::seconds(FLAGS_stateSubscriptionHeartbeat_s),
          FLAGS_trackMetadata,
          "fsdb",
          options_.serveIdPathSubs,
          true),
      operStatsStorage_(
          {},
          std::chrono::seconds(FLAGS_statsSubscriptionServe_s),
          std::chrono::seconds(FLAGS_statsSubscriptionHeartbeat_s),
          FLAGS_trackMetadata,
          "fsdb",
          options_.serveIdPathSubs) {
  num_instances_.incrementValue(1);

  initPerStreamCounters();

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
    const RawPathT& path,
    const PublisherId& publisherId,
    PubSubType type,
    bool isStats) {
  OperPublisherInfo info;
  info.publisherId() = publisherId;
  info.type() = type;
  info.path()->raw() = path;
  info.isStats() = isStats;
  try {
    auto pathConfig = fsdbConfig_->getPathConfig(publisherId, path);
    info.isExpectedPath() = *pathConfig.get().isExpected();
  } catch (const std::exception&) {
    // ignore exception if PathConfig is not available
  }
  return info;
}

void ServiceHandler::registerPublisher(const OperPublisherInfo& info) {
  XLOG(DBG2) << "Publisher connected " << *info.publisherId() << " : "
             << folly::join("/", *info.path()->raw());
  if (info.publisherId()->empty()) {
    throw Utils::createFsdbException(
        FsdbErrorCode::EMPTY_PUBLISHER_ID, "Publisher Id must not be empty");
  }
  auto key = ClientKey(
      *info.publisherId(), buildPathUnion(info), *info.type(), *info.isStats());
  auto resp = activePublishers_.wlock()->insert({std::move(key), info});
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
  num_publishers_.incrementValue(1);
  try {
    auto config =
        fsdbConfig_->getPathConfig(*info.publisherId(), *info.path()->raw());
    if (*config.get().isExpected()) {
      num_disconnected_publishers_.incrementValue(-1);
      auto counter = disconnectedPublishers_.find(
          PublisherKey(*info.publisherId(), *info.isStats()));
      if (counter != disconnectedPublishers_.end()) {
        counter->second.incrementValue(-1);
      }
    }
  } catch (const std::exception&) {
    // ignore exception if PathConfig is not available
  };
}

void ServiceHandler::unregisterPublisher(
    const OperPublisherInfo& info,
    FsdbErrorCode disconnectReason) {
  XLOG(DBG2) << " Publisher complete " << *info.publisherId() << " : "
             << folly::join("/", *info.path()->raw()) << " disconnectReason: "
             << apache::thrift::util::enumNameSafe(disconnectReason);
  auto key = ClientKey(
      *info.publisherId(), buildPathUnion(info), *info.type(), *info.isStats());
  activePublishers_.wlock()->erase(std::move(key));
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
  num_publishers_.incrementValue(-1);
  try {
    auto config =
        fsdbConfig_->getPathConfig(*info.publisherId(), *info.path()->raw());
    if (*config.get().isExpected()) {
      num_disconnected_publishers_.incrementValue(1);
      auto counter = disconnectedPublishers_.find(
          PublisherKey(*info.publisherId(), *info.isStats()));
      if (counter != disconnectedPublishers_.end()) {
        counter->second.incrementValue(1);
      }
    }
  } catch (const std::exception&) {
    // ignore exception if PathConfig is not available
  };
}

template <typename PubUnit>
apache::thrift::SinkConsumer<PubUnit, OperPubFinalResponse>
ServiceHandler::makeSinkConsumer(
    RawPathT&& path,
    const PublisherId& publisherId,
    bool isStats) {
  validateOperPublishPermissions(publisherId, path);
  PubSubType pubSubType{PubSubType::PATH};
  if constexpr (std::is_same_v<PubUnit, OperDelta>) {
    pubSubType = PubSubType::DELTA;
  } else if constexpr (std::is_same_v<PubUnit, PublisherMessage>) {
    pubSubType = PubSubType::PATCH;
  }

  auto info = makePublisherInfo(path, publisherId, pubSubType, isStats);
  registerPublisher(info);
  std::shared_ptr<FsdbErrorCode> disconnectReason =
      std::make_shared<FsdbErrorCode>(FsdbErrorCode::ALL_PUBLISHERS_GONE);
  auto cleanupPublisher =
      folly::makeGuard([this, info = std::move(info), disconnectReason]() {
        unregisterPublisher(info, *disconnectReason);
      });

  apache::thrift::SinkConsumer<PubUnit, OperPubFinalResponse> consumer{
      [this,
       publisherId,
       disconnectReason = std::move(disconnectReason),
       cleanupPublisher = std::move(cleanupPublisher),
       path = std::move(path),
       isStats](folly::coro::AsyncGenerator<PubUnit&&> gen)
          -> folly::coro::Task<OperPubFinalResponse> {
        OperPubFinalResponse finalResponse;
        try {
          while (auto chunk = co_await gen.next()) {
            XLOG(DBG5) << " chunk received";
            std::optional<StorageError> patchErr;
            if constexpr (std::is_same_v<PubUnit, OperState>) {
              updateMetadata(chunk->metadata().ensure());
              if (isStats) {
                patchErr = operStatsStorage_.set_encoded(
                    path.begin(), path.end(), *chunk);
              } else {
                patchErr =
                    operStorage_.set_encoded(path.begin(), path.end(), *chunk);
              }
            } else if constexpr (std::is_same_v<PubUnit, OperDelta>) {
              updateMetadata(chunk->metadata().ensure());
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
                patchErr = operStatsStorage_.patch(*chunk);
              } else {
                patchErr = operStorage_.patch(*chunk);
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
            } else if constexpr (std::is_same_v<PubUnit, PublisherMessage>) {
              // TODO: need to use the publish path
              auto patchChunk = chunk->move_patch();
              updateMetadata(*patchChunk.metadata());
              if (isStats) {
                patchErr = operStatsStorage_.patch(std::move(patchChunk));
              } else {
                patchErr = operStorage_.patch(std::move(patchChunk));
              }
            }
            XLOG(DBG5) << "Chunk patch result "
                       << (patchErr
                               ? fmt::format(
                                     "error: {}", fmt::underlying(*patchErr))
                               : "success");
          }
          co_return finalResponse;
        } catch (const fsdb::FsdbException& ex) {
          XLOG(ERR) << "Publisher " << publisherId
                    << " Server:sink: FsdbException: "
                    << apache::thrift::util::enumNameSafe(ex.get_errorCode())
                    << ": " << ex.get_message();
          *disconnectReason = ex.get_errorCode();
          throw;
        } catch (const std::exception& e) {
          XLOG(INFO) << "Publisher " << publisherId
                     << " Got exception in publish loop: " << e.what();
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
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
  PathValidator::validateStatePath(*request->path()->raw());
  co_return {
      {},
      makeSinkConsumer<OperState>(
          std::move(*request->path()->raw()), *request->publisherId(), false)};
}

folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
    OperPubInitResponse,
    OperState,
    OperPubFinalResponse>>
ServiceHandler::co_publishOperStatsPath(
    std::unique_ptr<OperPubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
  PathValidator::validateStatsPath(*request->path()->raw());
  co_return {
      {},
      makeSinkConsumer<OperState>(
          std::move(*request->path()->raw()), *request->publisherId(), true)};
}

folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
    OperPubInitResponse,
    OperDelta,
    OperPubFinalResponse>>
ServiceHandler::co_publishOperStateDelta(
    std::unique_ptr<OperPubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
  PathValidator::validateStatePath(*request->path()->raw());
  co_return {
      {},
      makeSinkConsumer<OperDelta>(
          std::move(*request->path()->raw()), *request->publisherId(), false)};
}

folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
    OperPubInitResponse,
    OperDelta,
    OperPubFinalResponse>>
ServiceHandler::co_publishOperStatsDelta(
    std::unique_ptr<OperPubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
  PathValidator::validateStatsPath(*request->path()->raw());
  co_return {
      {},
      makeSinkConsumer<OperDelta>(
          std::move(*request->path()->raw()), *request->publisherId(), true)};
}

folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
    OperPubInitResponse,
    PublisherMessage,
    OperPubFinalResponse>>
ServiceHandler::co_publishState(std::unique_ptr<PubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
  PathValidator::validateStatePath(*request->path()->path());
  co_return {
      {},
      makeSinkConsumer<PublisherMessage>(
          std::move(*request->path()->path()),
          *request->clientId()->instanceId(),
          false)};
}

folly::coro::Task<apache::thrift::ResponseAndSinkConsumer<
    OperPubInitResponse,
    PublisherMessage,
    OperPubFinalResponse>>
ServiceHandler::co_publishStats(std::unique_ptr<PubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
  PathValidator::validateStatePath(*request->path()->path());
  co_return {
      {},
      makeSinkConsumer<PublisherMessage>(
          std::move(*request->path()->path()),
          *request->clientId()->instanceId(),
          true)};
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

OperSubscriberInfo
makeSubscriberInfo(const SubRequest& req, PubSubType type, bool isStats) {
  OperSubscriberInfo info;
  info.subscriberId() = *req.clientId()->instanceId();
  info.type() = type;
  info.paths() = *req.paths();
  info.isStats() = isStats;
  return info;
}

void validatePaths(
    const std::map<SubscriptionKey, RawOperPath>& paths,
    bool isStats) {
  auto validatePath = isStats ? PathValidator::validateStatsPath
                              : PathValidator::validateStatePath;
  for (const auto& path : paths) {
    validatePath(*path.second.path());
  }
}

} // namespace

void ServiceHandler::updateSubscriptionCounters(
    const OperSubscriberInfo& info,
    bool isConnected) {
  auto connectedCountIncrement = isConnected ? 1 : -1;
  auto disconnectCountIncrement = isConnected ? -1 : 1;

  num_subscriptions_.incrementValue(connectedCountIncrement);

  auto config = fsdbConfig_->getSubscriberConfig(*info.subscriberId());
  if (config.has_value() && *config.value().second.get().trackReconnect()) {
    auto& clientId = config.value().first;
    num_disconnected_subscriptions_.incrementValue(disconnectCountIncrement);
    if (auto counter = disconnectedSubscriptions_.find(clientId);
        counter != disconnectedSubscriptions_.end()) {
      counter->second.incrementValue(disconnectCountIncrement);
    }
    if (auto counter = connectedSubscriptions_.find(clientId);
        counter != connectedSubscriptions_.end()) {
      counter->second.incrementValue(connectedCountIncrement);
      // per-subscriber counters: checks global subscription count
      int nSubscriptions{0};
      activeSubscriptions_.withRLock(
          [&clientId, &nSubscriptions](const auto& activeSubscriptions) {
            for (const auto& it : activeSubscriptions) {
              auto& subscription = it.second;
              if (clientId == *subscription.subscriberId()) {
                nSubscriptions++;
              }
            }
          });
      bool isFirstSubscriptionConnected = isConnected && nSubscriptions == 1;
      bool isLastSubscriptionDisconnected = !isConnected && nSubscriptions == 0;
      if (isFirstSubscriptionConnected || isLastSubscriptionDisconnected) {
        num_subscribers_.incrementValue(connectedCountIncrement);
        num_disconnected_subscribers_.incrementValue(disconnectCountIncrement);
        if (auto counter1 = disconnectedSubscribers_.find(clientId);
            counter1 != disconnectedSubscribers_.end()) {
          counter1->second.incrementValue(disconnectCountIncrement);
        }
      }
    }
  }
}

void ServiceHandler::registerSubscription(const OperSubscriberInfo& info) {
  if (info.subscriberId()->empty()) {
    throw Utils::createFsdbException(
        FsdbErrorCode::EMPTY_SUBSCRIBER_ID, "Subscriber Id must not be empty");
  }
  XLOG(INFO) << "Registering subscription " << *info.subscriberId();
  bool hasRawPath = info.path() && !info.path()->raw()->empty();
  bool hasExtendedPath = info.extendedPaths() && !info.extendedPaths()->empty();
  bool hasMultiPaths = info.paths() && !info.paths()->empty();
  validateSubscriptionPermissions(
      *info.subscriberId(),
      *info.type(),
      *info.isStats(),
      hasRawPath || hasMultiPaths,
      hasExtendedPath);
  auto key = ClientKey(
      *info.subscriberId(),
      buildPathUnion(info),
      *info.type(),
      *info.isStats());
  if (FLAGS_forceRegisterSubscriptions) {
    (*activeSubscriptions_.wlock())[std::move(key)] = info;
  } else {
    auto resp = activeSubscriptions_.wlock()->insert({std::move(key), info});
    if (!resp.second) {
      throw Utils::createFsdbException(
          FsdbErrorCode::ID_ALREADY_EXISTS,
          "Dup subscriber id: ",
          *info.subscriberId());
    }
  }
  updateSubscriptionCounters(info, true);
}
void ServiceHandler::unregisterSubscription(const OperSubscriberInfo& info) {
  std::string pathStr;
  // TODO: handle extended path to string
  if (info.path()) {
    pathStr = folly::join("/", *info.path()->raw());
  }
  XLOG(INFO) << "Subscription complete " << *info.subscriberId() << " : "
             << pathStr;
  auto key = ClientKey(
      *info.subscriberId(),
      buildPathUnion(info),
      *info.type(),
      *info.isStats());
  activeSubscriptions_.wlock()->erase(std::move(key));
  updateSubscriptionCounters(info, false);
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
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
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
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
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
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
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
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));

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
              std::transform(
                  std::make_move_iterator(deltas.begin()),
                  std::make_move_iterator(deltas.end()),
                  std::back_inserter(*unit.changes()),
                  [](auto&& delta) {
                    // we expect newVal to always be set, even in
                    // the case of a deleted path. For deleted
                    // paths, lower layers will create a
                    // TaggedOperState with empty contents.
                    return *std::move(delta.newVal);
                  });
              co_yield std::move(unit);
            }
          })};
}

folly::coro::Task<apache::thrift::ResponseAndServerStream<
    OperSubInitResponse,
    OperSubDeltaUnit>>
ServiceHandler::co_subscribeOperStateDeltaExtended(
    std::unique_ptr<OperSubRequestExtended> request) {
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));

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
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
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
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));

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
              std::transform(
                  std::make_move_iterator(deltas.begin()),
                  std::make_move_iterator(deltas.end()),
                  std::back_inserter(*unit.changes()),
                  [](auto&& delta) {
                    // we expect newVal to always be set, even in
                    // the case of a deleted path. For deleted
                    // paths, lower layers will create a
                    // TaggedOperState with empty contents.
                    return *std::move(delta.newVal);
                  });
              co_yield std::move(unit);
            }
          })};
}

folly::coro::Task<apache::thrift::ResponseAndServerStream<
    OperSubInitResponse,
    OperSubDeltaUnit>>
ServiceHandler::co_subscribeOperStatsDeltaExtended(
    std::unique_ptr<OperSubRequestExtended> request) {
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));

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

folly::coro::Task<apache::thrift::ResponseAndServerStream<
    OperSubInitResponse,
    SubscriberMessage>>
ServiceHandler::co_subscribeState(std::unique_ptr<SubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
  validatePaths(*request->paths(), false);
  auto subscriberInfo = makeSubscriberInfo(*request, PubSubType::PATCH, false);
  registerSubscription(subscriberInfo);
  auto cleanupSubscriber =
      folly::makeGuard([this, subscriberInfo = std::move(subscriberInfo)]() {
        unregisterSubscription(subscriberInfo);
      });
  auto stream = folly::coro::co_invoke(
      [this,
       request = std::move(request),
       cleanupSubscriber = std::move(cleanupSubscriber)]() mutable
      -> folly::coro::AsyncGenerator<SubscriberMessage&&> {
        return operStorage_.subscribe_patch(
            *request->clientId()->instanceId(), *request->paths());
      });
  co_return {{}, std::move(stream)};
}

folly::coro::Task<apache::thrift::ResponseAndServerStream<
    OperSubInitResponse,
    SubscriberMessage>>
ServiceHandler::co_subscribeStats(std::unique_ptr<SubRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
  validatePaths(*request->paths(), true);
  auto subscriberInfo = makeSubscriberInfo(*request, PubSubType::PATCH, true);
  registerSubscription(subscriberInfo);
  auto cleanupSubscriber =
      folly::makeGuard([this, subscriberInfo = std::move(subscriberInfo)]() {
        unregisterSubscription(subscriberInfo);
      });
  auto stream = folly::coro::co_invoke(
      [this,
       request = std::move(request),
       cleanupSubscriber = std::move(cleanupSubscriber)]() mutable
      -> folly::coro::AsyncGenerator<SubscriberMessage&&> {
        return operStatsStorage_.subscribe_patch(
            *request->clientId()->instanceId(), *request->paths());
      });
  co_return {{}, std::move(stream)};
}

folly::coro::Task<std::unique_ptr<OperState>> ServiceHandler::co_getOperState(
    std::unique_ptr<OperGetRequest> request) {
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
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
  auto log = LOG_THRIFT_CALL(INFO, getRequestDetails(*request));
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

folly::coro::Task<std::unique_ptr<PublisherIdToOperPublisherInfo>>
ServiceHandler::co_getAllOperPublisherInfos() {
  auto publishers = std::make_unique<PublisherIdToOperPublisherInfo>();
  activePublishers_.withRLock([&](const auto& activePublishers) {
    for (const auto& it : activePublishers) {
      auto& publisher = it.second;
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
    for (const auto& it : activePublishers) {
      auto& publisher = it.second;
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
  auto log = LOG_THRIFT_CALL(INFO);
  auto subscriptions = std::make_unique<SubscriberIdToOperSubscriberInfos>();
  activeSubscriptions_.withRLock([&](const auto& activeSubscriptions) {
    for (const auto& it : activeSubscriptions) {
      auto& subscription = it.second;
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
    for (const auto& it : activeSubscriptions) {
      auto& subscription = it.second;
      if (subscriberIds->find(*subscription.subscriberId()) ==
          subscriberIds->end()) {
        continue;
      }
      (*subscriptions)[*subscription.subscriberId()].push_back(subscription);
    }
  });
  co_return subscriptions;
}

void ServiceHandler::initPerStreamCounters(void) {
  for (const auto& [key, value] : *fsdbConfig_->getThrift().subscribers()) {
    if (value.trackReconnect().value()) {
      disconnectedSubscribers_.emplace(
          key,
          TLCounter(
              fb303::ThreadCachedServiceData::get()->getThreadStats(),
              folly::to<std::string>(
                  fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
                  "disconnected_subscriber.",
                  key)));
      auto count = value.numExpectedSubscriptions().value();
      if (count > 0) {
        if (auto counter = disconnectedSubscribers_.find(key);
            counter != disconnectedSubscribers_.end()) {
          counter->second.incrementValue(1);
        }
        num_disconnected_subscriptions_.incrementValue(count);
      }
      disconnectedSubscriptions_.emplace(
          key,
          TLCounter(
              fb303::ThreadCachedServiceData::get()->getThreadStats(),
              folly::to<std::string>(
                  fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
                  "disconnected_subscriptions.",
                  key)));
      if (auto counter = disconnectedSubscriptions_.find(key);
          counter != disconnectedSubscriptions_.end()) {
        counter->second.incrementValue(count);
      }
      connectedSubscriptions_.emplace(
          key,
          TLCounter(
              fb303::ThreadCachedServiceData::get()->getThreadStats(),
              folly::to<std::string>(
                  fsdb_common_constants::kFsdbServiceHandlerNativeStatsPrefix(),
                  "connected_subscriptions.",
                  key)));
    }
  }

  for (const auto& [key, value] : *fsdbConfig_->getThrift().publishers()) {
    for (const auto& pathConfig : *value.paths()) {
      if (*pathConfig.isExpected()) {
        PublisherKey publisherKey(key, *pathConfig.isStats());
        disconnectedPublishers_.emplace(
            publisherKey,
            TLCounter(
                fb303::ThreadCachedServiceData::get()->getThreadStats(),
                folly::to<std::string>(
                    fsdb_common_constants::
                        kFsdbServiceHandlerNativeStatsPrefix(),
                    "disconnected_publisher.",
                    key,
                    (*pathConfig.isStats() ? "-stats" : ""))));
        auto counter = disconnectedPublishers_.find(publisherKey);
        counter->second.incrementValue(1);
        num_disconnected_publishers_.incrementValue(1);
      }
    }
  }
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
      !*config.value().second.get().allowExtendedSubscriptions()) {
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
    const RawPathT& path) {
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
