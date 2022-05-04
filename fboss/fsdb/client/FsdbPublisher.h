// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Format.h>
#include <folly/String.h>
#include <folly/concurrency/DynamicBoundedQueue.h>
#include <folly/experimental/coro/AsyncGenerator.h>
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <atomic>
#include <shared_mutex>

namespace facebook::fboss::fsdb {
template <typename PubUnit>
class FsdbPublisher : public FsdbStreamClient {
  using QueueT = folly::DMPSCQueue<PubUnit, true /*may block*/>;
  static constexpr auto kPubQueueCapacity{2000};
  static std::unique_ptr<QueueT> makeQueue() {
    return std::make_unique<QueueT>(kPubQueueCapacity);
  }
  std::string typeStr() {
    return std::is_same_v<PubUnit, OperDelta> ? "Delta" : "Path";
  }

 public:
  FsdbPublisher(
      const std::string& clientId,
      const std::vector<std::string>& publishPath,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      bool publishStats,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {})
      : FsdbStreamClient(
            clientId,
            streamEvb,
            connRetryEvb,
            folly::sformat(
                "fsdb{}{}Publisher_{}",
                typeStr(),
                (publishStats ? "Stat" : "State"),
                folly::join('_', publishPath)),
            [this, stateChangeCb](State oldState, State newState) {
              if (newState == State::CONNECTED) {
                // For CONNECTED, first setup internal state and
                // then let clients know. Clients will want to
                // immediately start queueing up state post
                // CONNECTED, get publisher ready for that.
                handleStateChange(oldState, newState);
                stateChangeCb(oldState, newState);
              } else {
                // For state != CONNECTED, first clear out internal
                // state and then let clients know. This lets client
                // take any actions *before* publisher state is
                // cleared out.
                stateChangeCb(oldState, newState);
                handleStateChange(oldState, newState);
              }
            }),
        toPublishQueue_(nullptr),
        publishPath_(publishPath),
        publishStats_(publishStats),
        writeErrors_(
            fb303::ThreadCachedServiceData::get()->getThreadStats(),
            getCounterPrefix() + ".writeErrors",
            fb303::SUM,
            fb303::RATE) {}

  bool write(PubUnit pubUnit);

  ssize_t queueSize() const {
    auto toPublishQueueRPtr = toPublishQueue_.rlock();
    return *toPublishQueueRPtr ? (*toPublishQueueRPtr)->size() : 0;
  }
  size_t queueCapacity() const {
    return kPubQueueCapacity;
  }
  bool publishStats() const {
    return publishStats_;
  }

 protected:
#if FOLLY_HAS_COROUTINES
  folly::coro::AsyncGenerator<std::optional<PubUnit>> createGenerator();
#endif
  OperPubRequest createRequest() const;

 private:
  void handleStateChange(State oldState, State newState);
  // Note unique_ptr is synchronized, not QueueT. The latter manages its
  // own synchronization
  folly::Synchronized<std::unique_ptr<QueueT>> toPublishQueue_;
  const std::vector<std::string> publishPath_;
  const bool publishStats_;
  fb303::ThreadCachedServiceData::TLTimeseries writeErrors_;
};
} // namespace facebook::fboss::fsdb
