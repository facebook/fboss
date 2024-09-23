// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbSubscriber.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

template <typename MessageType, typename SubUnit, typename PathElement>
class FsdbPatchSubscriberImpl : public FsdbSubscriber<SubUnit, PathElement> {
  using BaseT = FsdbSubscriber<SubUnit, PathElement>;
  using SubUnitT = typename BaseT::SubUnitT;

 public:
  using FsdbOperPatchUpdateCb = typename BaseT::FsdbSubUnitUpdateCb;
  using BaseT::BaseT;
  using FsdbStreamClient::clientId;
  ~FsdbPatchSubscriberImpl() override {
    BaseT::cancel();
  }

 private:
#if FOLLY_HAS_COROUTINES
  using StreamT = typename BaseT::StreamT;
  using SubStreamT = typename BaseT::template SubStreamT<MessageType>;
  folly::coro::Task<StreamT> setupStream() override;
  folly::coro::Task<void> serveStream(StreamT&& stream) override;
#endif

  SubRequest createRequest() const;
};

using FsdbPatchSubscriber = FsdbPatchSubscriberImpl<
    SubscriberMessage,
    SubscriberChunk,
    std::map<SubscriptionKey, RawOperPath>>;
using FsdbExtPatchSubscriber = FsdbPatchSubscriberImpl<
    SubscriberMessage,
    SubscriberChunk,
    std::map<SubscriptionKey, ExtendedOperPath>>;
} // namespace facebook::fboss::fsdb
