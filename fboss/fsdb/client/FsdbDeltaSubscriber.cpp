// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"

#include <folly/logging/xlog.h>
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

namespace facebook::fboss::fsdb {

template <typename SubUnit, typename PathElement>
folly::coro::Task<
    typename FsdbDeltaSubscriberImpl<SubUnit, PathElement>::StreamT>
FsdbDeltaSubscriberImpl<SubUnit, PathElement>::setupStream() {
  auto initResponseReceiver =
      [&](const OperSubInitResponse& initResponse) -> bool {
    return !this->isCancelled();
  };

  // Workaround for GCC coroutine bug: Keep the request alive and materialize
  // the Task before co_await to prevent premature destruction.
  auto request = this->createRequest();

  if constexpr (std::is_same_v<SubUnit, OperDelta>) {
    auto task = this->isStats() ? this->client_->co_subscribeOperStatsDelta(
                                      this->getRpcOptions(), request)
                                : this->client_->co_subscribeOperStateDelta(
                                      this->getRpcOptions(), request);
    auto result = co_await std::move(task);
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  } else {
    auto task = this->isStats()
        ? this->client_->co_subscribeOperStatsDeltaExtended(
              this->getRpcOptions(), request)
        : this->client_->co_subscribeOperStateDeltaExtended(
              this->getRpcOptions(), request);
    auto result = co_await std::move(task);
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  }
}

template <typename SubUnit>
uint64_t getChunkMetadataPublishTime(const SubUnit& unit) {
  uint64_t lastPublishedAt = 0;
  if constexpr (std::is_same_v<SubUnit, OperDelta>) {
    if (unit.metadata().has_value()) {
      lastPublishedAt = unit.metadata()->lastPublishedAt().value_or(0);
    }
  } else if constexpr (std::is_same_v<SubUnit, OperSubDeltaUnit>) {
    if (unit.changes()->size() > 0) {
      if (unit.changes()[0].delta()->metadata().has_value()) {
        lastPublishedAt = unit.changes()[0]
                              .delta()
                              ->metadata()
                              .value()
                              .lastPublishedAt()
                              .value_or(0);
      }
    }
  }
  return lastPublishedAt;
}

template <typename SubUnit, typename PathElement>
folly::coro::Task<void>
FsdbDeltaSubscriberImpl<SubUnit, PathElement>::serveStream(StreamT&& stream) {
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
  while (auto delta = co_await gen.next()) {
    if (this->isCancelled()) {
      XLOG(DBG2) << " Detected cancellation: " << this->clientId();
      break;
    }

    if (BaseT::getSubscriptionState() == SubscriptionState::CONNECTED) {
      // compute publish-subscribe time latency
      auto publishTime = getChunkMetadataPublishTime(*delta);
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
      BaseT::updateSubscriptionState(SubscriptionState::CONNECTED);
    }
    if constexpr (std::is_same_v<SubUnit, OperDelta>) {
      std::optional<OperMetadata> metadata;
      if (delta->metadata().has_value()) {
        metadata = *delta->metadata();
      }
      this->onChunkReceived((!delta->changes()->size()), metadata);
    }
    if (!delta->changes()->size()) {
      continue;
    }
    if (this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
        this->getSubscriptionState() != SubscriptionState::CONNECTED) {
      BaseT::updateSubscriptionState(SubscriptionState::CONNECTED);
    }
    SubUnitT tmp(*delta);
    try {
      this->operSubUnitUpdate_(std::move(tmp));
    } catch (const std::exception& ex) {
      FsdbException e;
      e.message() = folly::exceptionStr(ex);
      e.errorCode() = FsdbErrorCode::SUBSCRIPTION_DATA_CALLBACK_ERROR;
      throw e;
    }
  }
  co_return;
}

template class FsdbDeltaSubscriberImpl<OperDelta, std::vector<std::string>>;
template class FsdbDeltaSubscriberImpl<
    OperSubDeltaUnit,
    std::vector<ExtendedOperPath>>;

} // namespace facebook::fboss::fsdb
