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

template <typename MessageType, typename SubUnit, typename PathElement>
folly::coro::Task<void>
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::serveStream(
    StreamT&& stream) {
  CHECK(std::holds_alternative<SubStreamT>(stream));
  auto gen = std::move(std::get<SubStreamT>(stream)).toAsyncGenerator();
  while (auto message = co_await gen.next()) {
    if (this->isCancelled()) {
      XLOG(DBG2) << " Detected cancellation: " << this->clientId();
      break;
    }
    if (!this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
        this->getSubscriptionState() != SubscriptionState::CONNECTED) {
      BaseT::updateSubscriptionState(SubscriptionState::CONNECTED);
    }
    switch (message->getType()) {
      case SubscriberMessage::Type::chunk:
        if (this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
            this->getSubscriptionState() != SubscriptionState::CONNECTED) {
          BaseT::updateSubscriptionState(SubscriptionState::CONNECTED);
        }
        this->operSubUnitUpdate_(message->move_chunk());
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
