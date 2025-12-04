// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaPublisher.h"
#include "fboss/fsdb/oper/DeltaValue.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

folly::coro::Task<FsdbDeltaPublisher::StreamT>
FsdbDeltaPublisher::setupStream() {
  auto initResponseReceiver =
      [&](const OperPubInitResponse& /* initResponse */) -> bool {
    return !isCancelled();
  };
  // Workaround for GCC coroutine bug: Keep the request alive and materialize
  // the Task before co_await to prevent premature destruction.
  auto request = createRequest();
  auto task = isStats() ? client_->co_publishOperStatsDelta(request)
                        : client_->co_publishOperStateDelta(request);
  auto result = co_await std::move(task);

  initResponseReceiver(result.response);

  co_return std::move(result.sink);
}

folly::coro::Task<void> FsdbDeltaPublisher::serveStream(StreamT&& stream) {
  CHECK(std::holds_alternative<DeltaPubStreamT>(stream));
  auto finalResponseReceiver =
      [&](const OperPubFinalResponse& /* finalResponse */) {};
  auto finalResponse = co_await std::get<DeltaPubStreamT>(stream).sink(
      [this]() -> folly::coro::AsyncGenerator<OperDelta&&> {
        auto gen = createGenerator();

        while (auto pubUnit = co_await gen.next()) {
          if (isCancelled()) {
            XLOG(DBG2) << " Detected cancellation";
            break;
          }
          if (isGracefulServiceLoopCompletionRequested()) {
            XLOG(ERR) << "Detected GR cancellation";
            throw FsdbClientGRDisconnectException(
                "DeltaPublisher disconnectReason: GR");
            break;
          }
          if (!pubUnit->metadata().has_value()) {
            pubUnit->metadata() = OperMetadata();
            pubUnit->metadata()->lastConfirmedAt() = lastConfirmedAt_;
            auto ts = std::chrono::system_clock::now().time_since_epoch();
            pubUnit->metadata()->lastPublishedAt() =
                std::chrono::duration_cast<std::chrono::milliseconds>(ts)
                    .count();
          } else {
            if (!initialSyncComplete_) {
              initialSyncComplete_ = true;
            }
            if (publishQueueMemoryLimit_ > 0) {
              size_t pubUnitSize = getPubUnitSize(*pubUnit);
              servedDataSize_.fetch_add(pubUnitSize);
            }
          }
          co_yield std::move(*pubUnit);
        }
        co_return;
      }());
  finalResponseReceiver(finalResponse);
  co_return;
}

size_t FsdbDeltaPublisher::getPubUnitSize(const OperDelta& delta) {
  return getOperDeltaSize(delta);
}

} // namespace facebook::fboss::fsdb
