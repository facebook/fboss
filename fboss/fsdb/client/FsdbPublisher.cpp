// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbPublisher.h"

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/logging/xlog.h>
#include <chrono>

namespace facebook::fboss::fsdb {

template <typename PubUnit>
OperPubRequest FsdbPublisher<PubUnit>::createRequest() const {
  OperPath operPath;
  operPath.raw() = publishPath_;
  OperPubRequest request;
  request.path() = operPath;
  request.publisherId() = clientId();
  return request;
}

template <typename PubUnit>
void FsdbPublisher<PubUnit>::handleStateChange(State oldState, State newState) {
  auto toPublishQueueWPtr = toPublishQueue_.wlock();
  if (newState != State::CONNECTED) {
    // If we went to any other state than CONNECTED, reset the publish queue.
    // Per FSDB protocol, publishers are required to do a full-sync post
    // reconnect, so there is no point storing previous states.
    toPublishQueueWPtr->reset();
  } else {
    (*toPublishQueueWPtr) = makeQueue();
  }
}
template <typename PubUnit>
void FsdbPublisher<PubUnit>::write(PubUnit pubUnit) {
  if (!pubUnit.metadata()) {
    pubUnit.metadata() = OperMetadata{};
  }
  if (!pubUnit.metadata()->lastConfirmedAt()) {
    auto now = std::chrono::system_clock::now();
    pubUnit.metadata()->lastConfirmedAt() =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();
  }
  auto toPublishQueueRPtr = toPublishQueue_.rlock();
  if (!(*toPublishQueueRPtr) ||
      !(*toPublishQueueRPtr)->try_enqueue(std::move(pubUnit))) {
    XLOG(ERR) << "Could not enqueue pub unit";
    FsdbException ex;
    ex.errorCode_ref() = FsdbErrorCode::DROPPED;
    ex.message_ref() = "Unable to queue pub unit";
    throw ex;
  }
}

#if FOLLY_HAS_COROUTINES
template <typename PubUnit>
folly::coro::AsyncGenerator<std::optional<PubUnit>>
FsdbPublisher<PubUnit>::createGenerator() {
  while (true) {
    PubUnit pubUnit;
    bool dequeued{false};
    {
      auto toPublishQueueRPtr = toPublishQueue_.rlock();
      if (*toPublishQueueRPtr) {
        dequeued =
            (*toPublishQueueRPtr)
                ->try_dequeue_for(pubUnit, std::chrono::milliseconds(10));
      } else {
        XLOG(ERR) << "Publish queue is null, unable to dequeue";
        FsdbException ex;
        ex.errorCode_ref() = FsdbErrorCode::DISCONNECTED;
        ex.message_ref() = "Publisher queue is null, cannot dequeue";
        throw ex;
      }
    }
    co_yield dequeued ? std::optional<PubUnit>(pubUnit) : std::nullopt;
  }
}
#endif

template class FsdbPublisher<OperDelta>;
template class FsdbPublisher<OperState>;

} // namespace facebook::fboss::fsdb
