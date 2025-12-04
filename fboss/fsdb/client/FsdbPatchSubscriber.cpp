// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPatchSubscriber.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

template <typename MessageType, typename SubUnit, typename PathElement>
SubRequest
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::createRequest()
    const {
  SubRequest request;
  request.clientId()->instanceId() = clientId();
  request.forceSubscribe() = this->subscriptionOptions().forceSubscribe_;
  if (this->subscriptionOptions().heartbeatInterval_.has_value()) {
    request.heartbeatInterval() =
        this->subscriptionOptions().heartbeatInterval_.value();
  }
  if constexpr (std::is_same_v<
                    PathElement,
                    std::map<SubscriptionKey, ExtendedOperPath>>) {
    request.extPaths() = this->subscribePaths();
  } else {
    request.paths() = this->subscribePaths();
  }
  return request;
}

template <typename MessageType, typename SubUnit, typename PathElement>
folly::coro::Task<
    typename FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::
        StreamT>
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::setupStream() {
  auto initResponseReceiver =
      [&](const OperSubInitResponse& initResponse) -> bool {
    return !this->isCancelled();
  };

  // Workaround for GCC coroutine bug: Keep the request alive and materialize
  // the Task before co_await to prevent premature destruction.
  auto request = this->createRequest();

  if constexpr (std::is_same_v<
                    PathElement,
                    std::map<SubscriptionKey, ExtendedOperPath>>) {
    auto task = this->isStats() ? this->client_->co_subscribeStatsExtended(
                                      this->getRpcOptions(), request)
                                : this->client_->co_subscribeStateExtended(
                                      this->getRpcOptions(), request);
    auto result = co_await std::move(task);
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  } else {
    auto task = this->isStats()
        ? this->client_->co_subscribeStats(this->getRpcOptions(), request)
        : this->client_->co_subscribeState(this->getRpcOptions(), request);
    auto result = co_await std::move(task);
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  }
}

std::optional<OperMetadata> getChunkMetadata(const SubscriberChunk& chunk) {
  std::optional<OperMetadata> metadata;
  if (chunk.patchGroups()->size() > 0) {
    for (const auto& [key, patchGroup] : *chunk.patchGroups()) {
      for (const auto& patch : patchGroup) {
        metadata = *patch.metadata();
        break;
      }
    }
  }
  return metadata;
}

uint64_t getChunkMetadataPublishTime(const SubscriberChunk& chunk) {
  auto md = getChunkMetadata(chunk);
  if (md.has_value()) {
    return md->lastPublishedAt().value_or(0);
  }
  return 0;
}

template <typename MessageType, typename SubUnit, typename PathElement>
folly::coro::Task<void>
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::serveStream(
    StreamT&& stream) {
  CHECK(std::holds_alternative<SubStreamT>(stream));
  // add histogram for publish-subscribe time latency
  // histogram range [0, 20s], 200ms width (100 bins)
  fb303::ThreadCachedServiceData::get()->addHistogram(
      this->clientPubsubLatencyMetric_, 200, 0, 2000);
  fb303::ThreadCachedServiceData::get()->exportHistogram(
      this->clientPubsubLatencyMetric_, 50, 95, 99);
  if (this->subscriptionOptions().exportPerSubscriptionMetrics_) {
    fb303::ThreadCachedServiceData::get()->addHistogram(
        this->subscribeLatencyMetric_, 200, 0, 2000);
    fb303::ThreadCachedServiceData::get()->exportHistogram(
        this->subscribeLatencyMetric_, 50, 95, 99);
  }
  auto gen = std::move(std::get<SubStreamT>(stream)).toAsyncGenerator();
  while (auto message = co_await gen.next()) {
    if (this->isCancelled()) {
      XLOG(DBG2) << " Detected cancellation: " << this->clientId();
      break;
    }

    bool hasData = (message->getType() == SubscriberMessage::Type::chunk);
    bool isInitialSync =
        (BaseT::getSubscriptionState() != SubscriptionState::CONNECTED);

    if (!isInitialSync && hasData) {
      // compute publish-subscribe time latency
      auto publishTime = getChunkMetadataPublishTime(message->get_chunk());
      if (publishTime > 0) {
        auto currentTimestamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        auto latency = currentTimestamp - publishTime;
        fb303::ThreadCachedServiceData::get()->addHistogramValue(
            this->clientPubsubLatencyMetric_, latency);
        if (this->subscriptionOptions().exportPerSubscriptionMetrics_) {
          fb303::ThreadCachedServiceData::get()->addHistogramValue(
              this->subscribeLatencyMetric_, latency);
        }
      }
    }

    if (!this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
        this->getSubscriptionState() != SubscriptionState::CONNECTED) {
      BaseT::updateSubscriptionState(SubscriptionState::CONNECTED, hasData);
    }
    switch (message->getType()) {
      case SubscriberMessage::Type::chunk:
        if (this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
            this->getSubscriptionState() != SubscriptionState::CONNECTED) {
          BaseT::updateSubscriptionState(SubscriptionState::CONNECTED, hasData);
        }
        this->onChunkReceived(false, getChunkMetadata(message->get_chunk()));
        try {
          this->operSubUnitUpdate_(message->move_chunk());
        } catch (const std::exception& ex) {
          FsdbException e;
          e.message() = folly::exceptionStr(ex);
          e.errorCode() = FsdbErrorCode::SUBSCRIPTION_DATA_CALLBACK_ERROR;
          throw e;
        }
        break;
      case SubscriberMessage::Type::heartbeat:
        if (message->get_heartbeat().metadata().has_value()) {
          this->onChunkReceived(
              true, message->get_heartbeat().metadata().value());
        } else {
          this->onChunkReceived(true, std::nullopt);
        }
        break;
      case SubscriberMessage::Type::__EMPTY__:
        break;
    }
  }
  co_return;
}

template class FsdbPatchSubscriberImpl<
    SubscriberMessage,
    SubscriberChunk,
    std::map<SubscriptionKey, RawOperPath>>;

template class FsdbPatchSubscriberImpl<
    SubscriberMessage,
    SubscriberChunk,
    std::map<SubscriptionKey, ExtendedOperPath>>;

} // namespace facebook::fboss::fsdb
