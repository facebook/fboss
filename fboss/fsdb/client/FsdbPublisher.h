// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Format.h>
#include <folly/String.h>
#include <folly/concurrency/DynamicBoundedQueue.h>
#include <folly/coro/AsyncGenerator.h>
#include <folly/coro/AsyncPipe.h>
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <atomic>
#include <shared_mutex>

namespace facebook::fboss::fsdb {
template <typename PubUnit>
class FsdbPublisher : public FsdbStreamClient {
  static constexpr auto kPubQueueCapacity{2000};
#if FOLLY_HAS_COROUTINES
  using PipeT =
      folly::coro::BoundedAsyncPipe<PubUnit, false /* SingleProducer */>;
  using GenT = folly::coro::AsyncGenerator<PubUnit&&>;
  using GenPipeT = std::pair<GenT, PipeT>;
  static std::unique_ptr<GenPipeT> makePipe() {
    return std::make_unique<GenPipeT>(PipeT::create(kPubQueueCapacity));
  }
#endif
  std::string typeStr();

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
            publishStats,
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
        publishPath_(publishPath),
#if FOLLY_HAS_COROUTINES
        asyncPipe_(makePipe()),
#endif
        writeErrors_(
            fb303::ThreadCachedServiceData::get()->getThreadStats(),
            getCounterPrefix() + ".writeErrors",
            fb303::SUM,
            fb303::RATE) {
  }

  bool write(PubUnit&& pubUnit);

  bool disconnectForGR();

  ssize_t queueSize() const {
    return queueSize_;
  }
  size_t queueCapacity() const {
    return kPubQueueCapacity;
  }

 protected:
#if FOLLY_HAS_COROUTINES
  folly::coro::AsyncGenerator<PubUnit&&> createGenerator();
#endif
  OperPubRequest createRequest() const;

  const std::vector<std::string> publishPath_;

 private:
  void handleStateChange(State oldState, State newState);
// Note unique_ptr is synchronized, not GenT/PipeT. The latter manages its
// own synchronization
#if FOLLY_HAS_COROUTINES
  folly::Synchronized<std::unique_ptr<GenPipeT>> asyncPipe_;
#endif
  std::atomic<ssize_t> queueSize_{0};
  fb303::ThreadCachedServiceData::TLTimeseries writeErrors_;
};
} // namespace facebook::fboss::fsdb
