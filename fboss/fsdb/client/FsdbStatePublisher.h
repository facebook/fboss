// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <utility>

#include "fboss/fsdb/client/FsdbPublisher.h"

namespace facebook::fboss::fsdb {
class FsdbStatePublisher : public FsdbPublisher<OperState> {
  using BaseT = FsdbPublisher<OperState>;

 public:
  FsdbStatePublisher(
      const std::string& clientId,
      const std::vector<std::string>& publishPath,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      bool publishStats,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {},
      std::optional<size_t> queueCapacity = std::nullopt)
      : BaseT(
            clientId,
            publishPath,
            streamEvb,
            connRetryEvb,
            publishStats,
            std::move(stateChangeCb),
            queueCapacity),
        coalescedUpdates_(
            fb303::ThreadCachedServiceData::get()->getThreadStats(),
            getCounterPrefix() + ".coalescedUpdates",
            fb303::SUM,
            fb303::RATE) {}

  ~FsdbStatePublisher() override {
    cancel();
  }

 protected:
  virtual bool tryWrite(
      folly::coro::BoundedAsyncPipe<OperState, false>& pipe,
      OperState&& pubUnit) override;

  virtual bool flush(
      folly::coro::BoundedAsyncPipe<OperState, false>& pipe) override;

 private:
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<StreamT> setupStream() override;
  folly::coro::Task<void> serveStream(StreamT&& stream) override;
#endif
  std::optional<OperState> nextUpdate_;
  fb303::ThreadCachedServiceData::TLTimeseries coalescedUpdates_;
};
} // namespace facebook::fboss::fsdb
