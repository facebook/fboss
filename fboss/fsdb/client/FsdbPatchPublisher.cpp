// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPatchPublisher.h"

#include <folly/logging/xlog.h>
#include <chrono>

namespace facebook::fboss::fsdb {

PubRequest FsdbPatchPublisher::createRequest() const {
  RawOperPath operPath;
  operPath.path() = publishPath_;
  PubRequest request;
  request.path() = operPath;
  request.clientId()->instanceId() = clientId();
  return request;
}

folly::coro::Task<FsdbPatchPublisher::StreamT>
FsdbPatchPublisher::setupStream() {
  auto initResponseReceiver =
      [&](const OperPubInitResponse& /* initResponse */) -> bool {
    return !isCancelled();
  };
  auto result = co_await (
      isStats() ? client_->co_publishStats(createRequest())
                : client_->co_publishState(createRequest()));
  initResponseReceiver(result.response);
  co_return std::move(result.sink);
}

folly::coro::Task<void> FsdbPatchPublisher::serveStream(StreamT&& stream) {
  CHECK(std::holds_alternative<PatchPubStreamT>(stream));
  auto finalResponseReceiver =
      [&](const OperPubFinalResponse& /* finalResponse */) {};
  auto finalResponse = co_await std::get<PatchPubStreamT>(stream).sink(
      [this]() -> folly::coro::AsyncGenerator<PublisherMessage&&> {
        auto gen = createGenerator();

        while (auto patch = co_await gen.next()) {
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
          PublisherMessage message;
          message.set_patch(std::move(*patch));
          co_yield std::move(message);
        }
        co_return;
      }());
  finalResponseReceiver(finalResponse);
  co_return;
}
} // namespace facebook::fboss::fsdb
