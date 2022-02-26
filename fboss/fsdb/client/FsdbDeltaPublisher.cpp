// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/FsdbDeltaPublisher.h"

#include <folly/experimental/coro/BlockingWait.h>
#include <folly/logging/xlog.h>
#include <chrono>
#include "fboss/fsdb/if/facebook/gen-cpp2/FsdbService.h"

namespace facebook::fboss::fsdb {

namespace {
OperPubRequest createRequest(const std::vector<std::string>& publishPath) {
  OperPath operPath;
  operPath.raw_ref() = publishPath;
  OperPubRequest request;
  request.path_ref() = operPath;
  return request;
}
} // namespace
folly::coro::AsyncGenerator<OperDelta> FsdbDeltaPublisher::createGenerator() {
  while (true) {
    OperDelta delta;
    toPublishQueue_.try_dequeue_for(delta, std::chrono::milliseconds(10));
    co_yield delta;
  }
}

folly::coro::Task<void> FsdbDeltaPublisher::serviceLoop() {
  try {
    auto initResponseReceiver =
        [&](const OperPubInitResponse& /* initResponse */) -> bool {
      return !isCancelled();
    };

    auto finalResponseReceiver =
        [&](const OperPubFinalResponse& /* finalResponse */) {};
    auto result =
        co_await client_->co_publishOperDelta(createRequest(publishPath_));
    if (initResponseReceiver(result.response)) {
      auto finalResponse = co_await result.sink.sink(
          [this]() -> folly::coro::AsyncGenerator<OperDelta&&> {
            auto gen = createGenerator();

            while (auto delta = co_await folly::coro::co_withCancellation(
                       (*cancelSource_.rlock())->getToken(), gen.next())) {
              if (isCancelled()) {
                XLOG(DBG2) << " Detected cancellation";
                break;
              }
              if (!delta->changes_ref()->size()) {
                continue;
              }
              OperDelta deltaValue = std::move(*delta);
              co_yield std::move(deltaValue);
            }
            co_return;
          }());
      finalResponseReceiver(finalResponse);
    }
  } catch (const folly::OperationCancelled&) {
    XLOG(DBG2) << "Packet Stream Operation cancelled";
  } catch (const std::exception& ex) {
    XLOG(ERR) << clientId() << " Server error: " << folly::exceptionStr(ex);
    setState(State::DISCONNECTED);
  }
  XLOG(DBG2) << "Client Cancellation Completed";
  co_return;
}
} // namespace facebook::fboss::fsdb
