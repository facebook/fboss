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
  if constexpr (std::is_same_v<SubUnit, OperState>) {
    auto result = co_await (
        this->isStats() ? this->client_->co_subscribeOperStatsPath(
                              this->getRpcOptions(), this->createRequest())
                        : this->client_->co_subscribeOperStatePath(
                              this->getRpcOptions(), this->createRequest()));
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  } else {
    auto result = co_await (
        this->isStats() ? this->client_->co_subscribeOperStatsPathExtended(
                              this->getRpcOptions(), this->createRequest())
                        : this->client_->co_subscribeOperStatePathExtended(
                              this->getRpcOptions(), this->createRequest()));
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
      this->subscribeLatencyMetric_, 200, 0, 2000);
  fb303::ThreadCachedServiceData::get()->exportHistogram(
      this->subscribeLatencyMetric_, 50, 95, 99);
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
            this->subscribeLatencyMetric_, latency);
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
