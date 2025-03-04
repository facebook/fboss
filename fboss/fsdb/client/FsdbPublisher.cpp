// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbPublisher.h"

#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/logging/xlog.h>
#include <chrono>
#include <memory>
#include <type_traits>
#include <utility>

namespace facebook::fboss::fsdb {

template <typename PubUnit>
std::string FsdbPublisher<PubUnit>::typeStr() {
  if constexpr (std::is_same_v<PubUnit, OperDelta>) {
    return "Delta";
  } else if constexpr (std::is_same_v<PubUnit, OperState>) {
    return "Path";
  } else {
    return "Patch";
  }
}

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
#if FOLLY_HAS_COROUTINES
  auto pipeWPtr = asyncPipe_.wlock();
  if (newState != State::CONNECTED) {
    // If we went to any other state than CONNECTED, reset the publish queue.
    // Per FSDB protocol, publishers are required to do a full-sync post
    // reconnect, so there is no point storing previous states.
    pipeWPtr->reset();
    queueSize_ = 0;
  } else {
    *pipeWPtr = makePipe();
  }
#endif
}
template <typename PubUnit>
bool FsdbPublisher<PubUnit>::write(PubUnit&& pubUnit) {
  pubUnit.metadata().ensure();
  {
    auto ts = std::chrono::system_clock::now().time_since_epoch();
    if (!pubUnit.metadata()->lastConfirmedAt()) {
      pubUnit.metadata()->lastConfirmedAt() =
          std::chrono::duration_cast<std::chrono::seconds>(ts).count();
    }
    if (!pubUnit.metadata()->lastPublishedAt()) {
      pubUnit.metadata()->lastPublishedAt() =
          std::chrono::duration_cast<std::chrono::milliseconds>(ts).count();
    }
  }
#if FOLLY_HAS_COROUTINES
  auto pipeUPtr = asyncPipe_.ulock();
  if (!(*pipeUPtr) || !(*pipeUPtr)->second.try_write(std::move(pubUnit))) {
    XLOG(ERR) << "Could not enqueue pub unit";
    if (*pipeUPtr) {
      XLOG(ERR) << "Queue overflow, reset queue pointer";
      // Reset queue pointer so the service loop breaks and we fall
      // back to full sync protocol
      pipeUPtr.moveFromUpgradeToWrite()->reset();
      queueSize_ = 0;
    }
    writeErrors_.addValue(1);
    return false;
  }
#endif
  queueSize_++;
  return true;
}

#if FOLLY_HAS_COROUTINES
template <typename PubUnit>
folly::coro::AsyncGenerator<PubUnit&&>
FsdbPublisher<PubUnit>::createGenerator() {
  while (true) {
    {
      auto pipeRPtr = asyncPipe_.rlock();
      if (*pipeRPtr) {
        auto pubUnit = co_await (*pipeRPtr)->first.next();
        if (!pubUnit) {
          continue;
        }
        queueSize_--;
        co_yield std::move(*pubUnit);
      } else {
        XLOG(ERR) << "Publish queue is null, unable to dequeue";
        FsdbException ex;
        ex.errorCode_ref() = FsdbErrorCode::DISCONNECTED;
        ex.message_ref() = "Publisher queue is null, cannot dequeue";
        co_yield folly::coro::co_error(ex);
      }
    }
  }
}
#endif

template <typename PubUnit>
bool FsdbPublisher<PubUnit>::disconnectForGR() {
#if FOLLY_HAS_COROUTINES
  // Mark the client as cancelling gracefully, and let the service loop know
  // that it should send GR disconnect.
  auto baton = folly::Baton<>();
  setGracefulServiceLoopCompletion([&baton]() { baton.post(); });
  // To inform the service loop, we will send an empty PubUnit to the pipe
  // as we can't write anything other than PubUnit to the pipe.
  {
    auto pipeUPtr = asyncPipe_.ulock();
    if (!(*pipeUPtr)) {
      XLOG(ERR) << "asyncPipe_.ulock() failed, nullptr";
      return false;
    }
    PubUnit emptyPubUnit;
    if (!(*pipeUPtr)->second.try_write(std::move(emptyPubUnit))) {
      XLOG(DBG2) << "pipe.try_write() failed, can ignore failure";
    }
  }
  // wait for the service loop to complete, and then cancel the client
  baton.wait();
  cancel();
#endif
  return true;
}

template class FsdbPublisher<OperDelta>;
template class FsdbPublisher<OperState>;
template class FsdbPublisher<Patch>;

} // namespace facebook::fboss::fsdb
