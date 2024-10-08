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
  if constexpr (std::is_same_v<SubUnit, OperDelta>) {
    auto result = co_await (
        this->isStats() ? this->client_->co_subscribeOperStatsDelta(
                              this->getRpcOptions(), this->createRequest())
                        : this->client_->co_subscribeOperStateDelta(
                              this->getRpcOptions(), this->createRequest()));
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  } else {
    auto result = co_await (
        this->isStats() ? this->client_->co_subscribeOperStatsDeltaExtended(
                              this->getRpcOptions(), this->createRequest())
                        : this->client_->co_subscribeOperStateDeltaExtended(
                              this->getRpcOptions(), this->createRequest()));
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  }
}

template <typename SubUnit, typename PathElement>
folly::coro::Task<void>
FsdbDeltaSubscriberImpl<SubUnit, PathElement>::serveStream(StreamT&& stream) {
  CHECK(std::holds_alternative<SubStreamT>(stream));
  auto gen = std::move(std::get<SubStreamT>(stream)).toAsyncGenerator();
  while (auto delta = co_await gen.next()) {
    if (this->isCancelled()) {
      XLOG(DBG2) << " Detected cancellation: " << this->clientId();
      break;
    }
    if (!this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
        this->getSubscriptionState() != SubscriptionState::CONNECTED) {
      BaseT::updateSubscriptionState(SubscriptionState::CONNECTED);
    }
    if (!delta->changes()->size()) {
      continue;
    }
    if (this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
        this->getSubscriptionState() != SubscriptionState::CONNECTED) {
      BaseT::updateSubscriptionState(SubscriptionState::CONNECTED);
    }
    SubUnitT tmp(*delta);
    this->operSubUnitUpdate_(std::move(tmp));
  }
  co_return;
}

template class FsdbDeltaSubscriberImpl<OperDelta, std::vector<std::string>>;
template class FsdbDeltaSubscriberImpl<
    OperSubDeltaUnit,
    std::vector<ExtendedOperPath>>;

} // namespace facebook::fboss::fsdb
