// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbSubscriber.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {
template <typename SubUnit, typename PathElement>
class FsdbStateSubscriberImpl : public FsdbSubscriber<SubUnit, PathElement> {
  using BaseT = FsdbSubscriber<SubUnit, PathElement>;
  using SubUnitT = typename BaseT::SubUnitT;

 public:
  using FsdbOperStateUpdateCb = typename BaseT::FsdbSubUnitUpdateCb;
  using BaseT::BaseT;
  ~FsdbStateSubscriberImpl() override {
    this->cancel();
  }

 private:
#if FOLLY_HAS_COROUTINES
  using StreamT = typename BaseT::StreamT;
  using SubStreamT = typename BaseT::template SubStreamT<SubUnit>;
  folly::coro::Task<StreamT> setupStream() override;
  folly::coro::Task<void> serveStream(StreamT&& stream) override;
#endif
};
using FsdbStateSubscriber =
    FsdbStateSubscriberImpl<OperState, std::vector<std::string>>;
using FsdbExtStateSubscriber =
    FsdbStateSubscriberImpl<OperSubPathUnit, std::vector<ExtendedOperPath>>;
} // namespace facebook::fboss::fsdb
