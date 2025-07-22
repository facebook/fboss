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
  auto result = co_await (
      isStats() ? client_->co_publishOperStatsPath(createRequest())
                : client_->co_publishOperStatePath(createRequest()));
  initResponseReceiver(result.response);
  co_return std::move(result.sink);
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
