// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbStateSubscriber.h"

#include <folly/logging/xlog.h>
#include <type_traits>
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

namespace facebook::fboss::fsdb {

template <typename SubUnit, typename PathElement>
folly::coro::Task<
    typename FsdbStateSubscriberImpl<SubUnit, PathElement>::StreamT>
FsdbStateSubscriberImpl<SubUnit, PathElement>::setupStream() {
  auto initResponseReceiver =
      [&](const OperSubInitResponse& /* initResponse */) -> bool {
    return !this->isCancelled();
  };

  // Workaround for GCC coroutine bug: Keep the request alive and materialize
  // the Task before co_await to prevent premature destruction.
  auto request = this->createRequest();

  if constexpr (std::is_same_v<SubUnit, OperState>) {
    auto task = this->isStats() ? this->client_->co_subscribeOperStatsPath(
                                      this->getRpcOptions(), request)
                                : this->client_->co_subscribeOperStatePath(
                                      this->getRpcOptions(), request);
    auto result = co_await std::move(task);
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  } else {
    auto task = this->isStats()
        ? this->client_->co_subscribeOperStatsPathExtended(
              this->getRpcOptions(), request)
        : this->client_->co_subscribeOperStatePathExtended(
              this->getRpcOptions(), request);
    auto result = co_await std::move(task);
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  }
}

template <typename SubUnit, typename PathElement>
folly::coro::Task<void>
FsdbStateSubscriberImpl<SubUnit, PathElement>::serveStream(StreamT&& stream) {
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
  while (auto state = co_await gen.next()) {
    if (this->isCancelled()) {
      XLOG(DBG2) << " Detected cancellation: " << this->clientId();
      break;
    }
    if (!this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
        this->getSubscriptionState() != SubscriptionState::CONNECTED) {
      BaseT::updateSubscriptionState(SubscriptionState::CONNECTED);
    }

    uint64_t lastPublishedAt = 0;
    if constexpr (std::is_same_v<SubUnitT, OperState>) {
      std::optional<OperMetadata> metadata;
      if (state->metadata().has_value()) {
        metadata = *state->metadata();
      }
      this->onChunkReceived(*state->isHeartbeat(), metadata);
      if (*state->isHeartbeat()) {
        continue;
      }
      if (state->metadata().has_value()) {
        lastPublishedAt = state->metadata()->lastPublishedAt().value_or(0);
      }
    } else {
      if (!state->changes()->size()) {
        continue;
      }
      if (state->changes()[0].state()->metadata().has_value()) {
        lastPublishedAt = state->changes()[0]
                              .state()
                              ->metadata()
                              .value()
                              .lastPublishedAt()
                              .value_or(0);
      }
    }

    if (BaseT::getSubscriptionState() == SubscriptionState::CONNECTED) {
      // compute publish-subscribe time latency
      if (lastPublishedAt > 0) {
        auto currentTimestamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        auto latency = currentTimestamp - lastPublishedAt;
        fb303::ThreadCachedServiceData::get()->addHistogramValue(
            this->clientPubsubLatencyMetric_, latency);
        if (this->subscriptionOptions().exportPerSubscriptionMetrics_) {
          fb303::ThreadCachedServiceData::get()->addHistogramValue(
              this->subscribeLatencyMetric_, latency);
        }
      }
    }

    if (this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
        this->getSubscriptionState() != SubscriptionState::CONNECTED) {
      BaseT::updateSubscriptionState(SubscriptionState::CONNECTED);
    }
    SubUnitT tmp(*state);
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

template class FsdbStateSubscriberImpl<OperState, std::vector<std::string>>;
template class FsdbStateSubscriberImpl<
    OperSubPathUnit,
    std::vector<ExtendedOperPath>>;
} // namespace facebook::fboss::fsdb
