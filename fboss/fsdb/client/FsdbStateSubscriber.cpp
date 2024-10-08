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
    if constexpr (std::is_same_v<SubUnitT, OperState>) {
      if (*state->isHeartbeat()) {
        continue;
      }
    } else {
      if (!state->changes()->size()) {
        continue;
      }
    }
    if (this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
        this->getSubscriptionState() != SubscriptionState::CONNECTED) {
      BaseT::updateSubscriptionState(SubscriptionState::CONNECTED);
    }
    SubUnitT tmp(*state);
    this->operSubUnitUpdate_(std::move(tmp));
  }
  co_return;
}

template class FsdbStateSubscriberImpl<OperState, std::vector<std::string>>;
template class FsdbStateSubscriberImpl<
    OperSubPathUnit,
    std::vector<ExtendedOperPath>>;
} // namespace facebook::fboss::fsdb
