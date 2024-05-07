// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbPublisher.h"

namespace facebook::fboss::fsdb {
class FsdbStatePublisher : public FsdbPublisher<OperState> {
  using BaseT = FsdbPublisher<OperState>;

 public:
  using BaseT::BaseT;
  ~FsdbStatePublisher() override {
    cancel();
  }

 private:
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<StreamT> setupStream() override;
  folly::coro::Task<void> serveStream(StreamT&& stream) override;
#endif
};
} // namespace facebook::fboss::fsdb
