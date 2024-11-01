// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaPublisher.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

folly::coro::Task<FsdbDeltaPublisher::StreamT>
FsdbDeltaPublisher::setupStream() {
  auto initResponseReceiver =
      [&](const OperPubInitResponse& /* initResponse */) -> bool {
    return !isCancelled();
  };
  auto result = co_await (
      isStats() ? client_->co_publishOperStatsDelta(createRequest())
                : client_->co_publishOperStateDelta(createRequest()));
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
          if (!pubUnit->changes()->size()) {
            continue;
          }
          co_yield std::move(*pubUnit);
        }
        co_return;
      }());
  finalResponseReceiver(finalResponse);
  co_return;
}
} // namespace facebook::fboss::fsdb
