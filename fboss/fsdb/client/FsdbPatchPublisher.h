// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbPublisher.h"

namespace facebook::fboss::fsdb {
class FsdbPatchPublisher : public FsdbPublisher<Patch> {
  using BaseT = FsdbPublisher<Patch>;

 public:
  using BaseT::BaseT;
  ~FsdbPatchPublisher() override {
    cancel();
  }

 protected:
  size_t getPubUnitSize(const Patch& patch) override;

 private:
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<StreamT> setupStream() override;
  folly::coro::Task<void> serveStream(StreamT&& stream) override;
#endif

  PubRequest createRequest() const;
};
} // namespace facebook::fboss::fsdb
