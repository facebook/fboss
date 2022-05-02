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
void FsdbPublisher<PubUnit>::handleStateChange(
    State /*oldState*/,
    State newState) {
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
bool FsdbPublisher<PubUnit>::write(PubUnit pubUnit) {
  if (!pubUnit.metadata()) {
    pubUnit.metadata() = OperMetadata{};
  }
  if (!pubUnit.metadata()->lastConfirmedAt()) {
    auto now = std::chrono::system_clock::now();
    pubUnit.metadata()->lastConfirmedAt() =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();
  }
  auto toPublishQueueUPtr = toPublishQueue_.ulock();
  if (!(*toPublishQueueUPtr) ||
      !(*toPublishQueueUPtr)->try_enqueue(std::move(pubUnit))) {
    XLOG(ERR) << "Could not enqueue pub unit";
    if (*toPublishQueueUPtr) {
      XLOG(ERR) << "Queue overflow, reset queue pointer";
      // Reset queue pointer so the service loop breaks and we fall
      // back to full sync protocol
      toPublishQueueUPtr.moveFromUpgradeToWrite()->reset();
    }
    writeErrors_.addValue(1);
    return false;
  }
  return true;
}

#if FOLLY_HAS_COROUTINES
template <typename PubUnit>
folly::coro::AsyncGenerator<std::optional<PubUnit>>
FsdbPublisher<PubUnit>::createGenerator() {
  while (true) {
    {
      auto toPublishQueueRPtr = toPublishQueue_.rlock();
      if (*toPublishQueueRPtr) {
        PubUnit pubUnit;
        co_yield(*toPublishQueueRPtr)
                ->try_dequeue_for(pubUnit, std::chrono::milliseconds(10))
            ? std::optional<PubUnit>(pubUnit)
            : std::nullopt;
      } else {
        XLOG(ERR) << "Publish queue is null, unable to dequeue";
        FsdbException ex;
        ex.errorCode_ref() = FsdbErrorCode::DISCONNECTED;
        ex.message_ref() = "Publisher queue is null, cannot dequeue";
        throw ex;
      }
    }
  }
}
#endif

template class FsdbPublisher<OperDelta>;
template class FsdbPublisher<OperState>;

} // namespace facebook::fboss::fsdb
