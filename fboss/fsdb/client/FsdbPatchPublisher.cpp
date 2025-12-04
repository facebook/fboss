// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPatchPublisher.h"
#include "fboss/fsdb/oper/DeltaValue.h"

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
  // Workaround for GCC coroutine bug: Keep the request alive and materialize
  // the Task before co_await to prevent premature destruction.
  auto request = createRequest();
  auto task = isStats() ? client_->co_publishStats(request)
                        : client_->co_publishState(request);
  auto result = co_await std::move(task);

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
          if (!patch->metadata()->lastPublishedAt().has_value()) {
            Heartbeat heartbeat;
            auto ts = std::chrono::system_clock::now().time_since_epoch();
            heartbeat.metadata() = OperMetadata();
            heartbeat.metadata()->lastConfirmedAt() = lastConfirmedAt_;
            heartbeat.metadata()->lastPublishedAt() =
                std::chrono::duration_cast<std::chrono::milliseconds>(ts)
                    .count();
            message.set_heartbeat(std::move(heartbeat));
          } else {
            if (publishQueueMemoryLimit_ > 0) {
              size_t pubUnitSize = getPubUnitSize(*patch);
              servedDataSize_.fetch_add(pubUnitSize);
            }
            message.set_patch(std::move(*patch));
            if (!initialSyncComplete_) {
              initialSyncComplete_ = true;
            }
          }
          co_yield std::move(message);
        }
        co_return;
      }());
  finalResponseReceiver(finalResponse);
  co_return;
}

size_t FsdbPatchPublisher::getPubUnitSize(const Patch& patch) {
  return getPatchNodeSize(*patch.patch());
}

} // namespace facebook::fboss::fsdb
