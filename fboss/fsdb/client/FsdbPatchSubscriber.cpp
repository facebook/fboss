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
  RawOperPath path;
  request.paths() = this->subscribePaths();
  request.forceSubscribe() = this->subscriptionOptions().forceSubscribe_;
  if (this->subscriptionOptions().heartbeatInterval_.has_value()) {
    request.heartbeatInterval() =
        this->subscriptionOptions().heartbeatInterval_.value();
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
  auto result = co_await (
      this->isStats() ? this->client_->co_subscribeStats(
                            this->getRpcOptions(), this->createRequest())
                      : this->client_->co_subscribeState(
                            this->getRpcOptions(), this->createRequest()));
  initResponseReceiver(result.response);
  co_return std::move(result.stream);
}

uint64_t getChunkMetadataPublishTime(const SubscriberChunk& chunk) {
  uint64_t lastPublishedAt = 0;
  if (chunk.patchGroups()->size() > 0) {
    for (const auto& [key, patchGroup] : *chunk.patchGroups()) {
      for (const auto& patch : patchGroup) {
        if (patch.metadata()->lastPublishedAt().has_value()) {
          lastPublishedAt = patch.metadata()->lastPublishedAt().value();
          break;
        }
      }
    }
  }
  return lastPublishedAt;
}

template <typename MessageType, typename SubUnit, typename PathElement>
folly::coro::Task<void>
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::serveStream(
    StreamT&& stream) {
  CHECK(std::holds_alternative<SubStreamT>(stream));
  // add histogram for publish-subscribe time latency
  // histogram range [0, 20s], 200ms width (100 bins)
  fb303::ThreadCachedServiceData::get()->addHistogram(
      this->subscribeLatencyMetric_, 200, 0, 2000);
  fb303::ThreadCachedServiceData::get()->exportHistogram(
      this->subscribeLatencyMetric_, 50, 95, 99);
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
            this->subscribeLatencyMetric_, latency);
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

} // namespace facebook::fboss::fsdb
