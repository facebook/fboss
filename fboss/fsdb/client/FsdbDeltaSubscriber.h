// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbSubscriber.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

template <typename SubUnit, typename PathElement>
class FsdbDeltaSubscriberImpl : public FsdbSubscriber<SubUnit, PathElement> {
  using BaseT = FsdbSubscriber<SubUnit, PathElement>;
  using SubUnitT = typename BaseT::SubUnitT;

 public:
  using FsdbOperDeltaUpdateCb = typename BaseT::FsdbSubUnitUpdateCb;
  using BaseT::BaseT;
  ~FsdbDeltaSubscriberImpl() override {
    BaseT::cancel();
  }
  FsdbDeltaSubscriberImpl(const FsdbDeltaSubscriberImpl&) = delete;
  FsdbDeltaSubscriberImpl& operator=(const FsdbDeltaSubscriberImpl&) = delete;
  FsdbDeltaSubscriberImpl(FsdbDeltaSubscriberImpl&&) = delete;
  FsdbDeltaSubscriberImpl& operator=(FsdbDeltaSubscriberImpl&&) = delete;

 private:
#if FOLLY_HAS_COROUTINES
  using StreamT = typename BaseT::StreamT;
  using SubStreamT = typename BaseT::template SubStreamT<SubUnit>;
  folly::coro::Task<StreamT> setupStream() override;
  folly::coro::Task<void> serveStream(StreamT&& stream) override;
#endif
};

using FsdbDeltaSubscriber =
    FsdbDeltaSubscriberImpl<OperDelta, std::vector<std::string>>;
using FsdbExtDeltaSubscriber =
    FsdbDeltaSubscriberImpl<OperSubDeltaUnit, std::vector<ExtendedOperPath>>;
} // namespace facebook::fboss::fsdb
