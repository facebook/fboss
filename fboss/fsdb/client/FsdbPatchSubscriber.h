// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbSubscriber.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/Synchronized.h>

#include <cstdint>
#include <optional>

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
  FsdbPatchSubscriberImpl(const FsdbPatchSubscriberImpl&) = delete;
  FsdbPatchSubscriberImpl& operator=(const FsdbPatchSubscriberImpl&) = delete;
  FsdbPatchSubscriberImpl(FsdbPatchSubscriberImpl&&) = delete;
  FsdbPatchSubscriberImpl& operator=(FsdbPatchSubscriberImpl&&) = delete;

  // Append paths to this live patch subscription. Paths are recorded so a
  // reconnect re-subscribes the full (original + added) set (guaranteed
  // eventual delivery); if connected, a best-effort async RPC also extends the
  // live server-side subscription for immediate initial sync. Always returns
  // std::nullopt today (raw and extended paths both accepted; server-side
  // rejection surfaces only as a logged warning); the FsdbErrorCode return is
  // retained for a future synchronous client-side rejection.
  std::optional<FsdbErrorCode> addPaths(const PathElement& newPaths);

 private:
#if FOLLY_HAS_COROUTINES
  using StreamT = typename BaseT::StreamT;
  using SubStreamT = typename BaseT::template SubStreamT<MessageType>;
  folly::coro::Task<StreamT> setupStream() override;
  folly::coro::Task<void> serveStream(StreamT&& stream) override;
#endif

  SubRequest createRequest() const;

  // Server-assigned uid for the current subscription, captured from
  // OperSubInitResponse and refreshed on every (re)connect.
  folly::Synchronized<std::optional<uint64_t>> serverUid_;
  // Paths appended post-subscribe, merged into the re-subscribe request so they
  // survive reconnects.
  folly::Synchronized<PathElement> addedPaths_;
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
