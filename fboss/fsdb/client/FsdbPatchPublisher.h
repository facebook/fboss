// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbPublisher.h"

namespace facebook::fboss::fsdb {
class FsdbPatchPublisher : public FsdbPublisher<thrift_cow::Patch> {
  using BaseT = FsdbPublisher<thrift_cow::Patch>;

 public:
  using BaseT::BaseT;
  ~FsdbPatchPublisher() override {
    cancel();
  }

 private:
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<StreamT> setupStream() override;
  folly::coro::Task<void> serveStream(StreamT&& stream) override;
#endif

  PubRequest createRequest() const;
};
} // namespace facebook::fboss::fsdb
