// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbSubscriber.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {
class FsdbStateSubscriber : public FsdbSubscriber<OperState, std::string> {
  using BaseT = FsdbSubscriber<OperState, std::string>;
  using SubUnitT = typename BaseT::SubUnitT;

 public:
  using FsdbOperStateUpdateCb = typename BaseT::FsdbSubUnitUpdateCb;
  using BaseT::BaseT;
  ~FsdbStateSubscriber() override {
    cancel();
  }

 private:
#if FOLLY_HAS_COROUTINES && !defined(IS_OSS)
  folly::coro::Task<StreamT> setupStream() override;
  folly::coro::Task<void> serveStream(StreamT&& stream) override;
#endif
};
} // namespace facebook::fboss::fsdb
