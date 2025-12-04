// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbStatePublisher.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

folly::coro::Task<FsdbStatePublisher::StreamT>
FsdbStatePublisher::setupStream() {
  auto initResponseReceiver =
      [&](const OperPubInitResponse& /* initResponse */) -> bool {
    return !isCancelled();
  };
  // Workaround for GCC coroutine bug: Keep the request alive and materialize
  // the Task before co_await to prevent premature destruction.
  auto request = createRequest();
  auto task = isStats() ? client_->co_publishOperStatsPath(request)
                        : client_->co_publishOperStatePath(request);
  auto result = co_await std::move(task);

  initResponseReceiver(result.response);

  co_return std::move(result.sink);
}

bool FsdbStatePublisher::tryWrite(
    folly::coro::BoundedAsyncPipe<OperState, false>& pipe,
    OperState&& pubUnit) {
  if (nextUpdate_.has_value()) {
    coalescedUpdates_.addValue(1);
  }
  nextUpdate_ = std::move(pubUnit);
  return flush(pipe);
}

bool FsdbStatePublisher::flush(
    folly::coro::BoundedAsyncPipe<OperState, false>& pipe) {
  if (!nextUpdate_.has_value()) {
    return true;
  }

  auto queuedChunks = pipe.getOccupiedSpace();
  if (queuedChunks > 1) {
    XLOG(DBG2) << "fsdb publish stream has pending chunks, skip flush";
    return true;
  }

  if (pipe.try_write(*nextUpdate_)) {
    nextUpdate_.reset();
    chunksWritten_.addValue(1);
    return true;
  } else {
    return false;
  }
}

folly::coro::Task<void> FsdbStatePublisher::serveStream(StreamT&& stream) {
  CHECK(std::holds_alternative<StatePubStreamT>(stream));
  auto finalResponseReceiver =
      [&](const OperPubFinalResponse& /* finalResponse */) {};
  auto finalResponse = co_await std::get<StatePubStreamT>(stream).sink(
      [this]() -> folly::coro::AsyncGenerator<OperState&&> {
        auto gen = createGenerator();

        while (auto pubUnit = co_await gen.next()) {
          if (isCancelled()) {
            XLOG(DBG2) << " Detected cancellation";
            break;
          }
          if (isGracefulServiceLoopCompletionRequested()) {
            XLOG(ERR) << "Detected GR cancellation";
            throw FsdbClientGRDisconnectException(
                "StatePublisher disconnectReason: GR");
            break;
          }
          if (pubUnit && !pubUnit->metadata().has_value()) {
            pubUnit->isHeartbeat() = true;
            pubUnit->metadata() = OperMetadata();
            pubUnit->metadata()->lastConfirmedAt() = lastConfirmedAt_;
            pubUnit->metadata()->lastPublishedAt() =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
          } else if (!initialSyncComplete_) {
            initialSyncComplete_ = true;
          }
          if (pubUnit) {
            co_yield *pubUnit;
          }
        }
        co_return;
      }());
  finalResponseReceiver(finalResponse);
  co_return;
}
} // namespace facebook::fboss::fsdb
