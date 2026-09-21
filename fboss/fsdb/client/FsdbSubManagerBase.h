// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbPatchSubscriber.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

#include <folly/io/async/ScopedEventBaseThread.h>

namespace facebook::fboss::fsdb {

auto constexpr kReconnectThread = "FsdbReconnectThread";
auto constexpr kSubscriberThread = "FsdbSubscriberThread";

/*
 * Base class for FsdbSubManager that contains non-templated functionality.
 * This class handles the common subscription management logic that doesn't
 * depend on the specific template parameters.
 */
class FsdbSubManagerBase {
 public:
  virtual ~FsdbSubManagerBase();

  virtual void stop();

  /*
   * Force a transient disconnect of the underlying FSDB stream. The
   * subscriber will automatically reconnect using the existing backoff.
   * Safe to call after stop() (no-op).
   *
   * When noGR is true and the subscription was created with
   * grHoldTimeSec > 0, the post-disconnect SubscriptionState transition skips
   * DISCONNECTED_GR_HOLD and lands directly on DISCONNECTED_GR_HOLD_EXPIRED
   * so consumers gated on isGRHoldExpired() drop stale state immediately.
   * On non-GR subscriptions noGR is a no-op.
   */
  virtual void reconnect(bool noGR = false);

  const std::string& clientId() const;

  std::optional<SubscriptionInfo> getInfo();

  // Whether the FSDB server on the current connection supports adding paths to
  // a live subscription -- i.e. a path passed to addPathToLiveSubscription is
  // extended in place for an immediate initial sync, rather than being deferred
  // to the next (re)connect. Returns false when there is no active
  // subscription, before the connection's initial sync completes, or when
  // connected to an older server that does not advertise the capability.
  // Callers can branch on this to fall back to reconnect()/re-subscribe on
  // older servers.
  bool supportsLiveAddPath() const;

 protected:
  FsdbSubManagerBase(
      fsdb::SubscriptionOptions opts,
      utils::ConnectionOptions serverOptions,
      folly::EventBase* reconnectEvb,
      folly::EventBase* subscriberEvb);

  SubscriptionKey addPathImpl(const std::vector<std::string>& pathTokens);

  // Append a path to an already-subscribed (raw-path) subscription. Unlike
  // addPathImpl, this is valid only after subscribe(). Throws FsdbException
  // only on a client-side validation error (e.g., empty path tokens).
  // Server-side rejections are logged and delivered via the reconnect merge,
  // not surfaced as an exception through this call.
  SubscriptionKey addPathToLiveSubscriptionImpl(
      const std::vector<std::string>& pathTokens);

  SubscriptionKey addExtendedPathImpl(
      const std::vector<OperPathElem>& pathTokens);

  // Append an extended path to an already-subscribed (extended-path)
  // subscription. Valid only after subscribe(). Throws FsdbException on error.
  SubscriptionKey addExtendedPathToLiveSubscriptionImpl(
      const std::vector<OperPathElem>& pathTokens);

  void subscribeImpl(
      std::function<void(SubscriberChunk&&)> chunkHandler,
      std::optional<SubscriptionStateChangeCb> subscriptionStateChangeCb,
      std::optional<FsdbStreamHeartbeatCb> heartbeatCb);

  fsdb::SubscriptionOptions opts_;
  utils::ConnectionOptions connectionOptions_;
  std::unique_ptr<folly::ScopedEventBaseThread> reconnectEvbThread_{nullptr};
  std::unique_ptr<folly::ScopedEventBaseThread> subscriberEvbThread_{nullptr};
  folly::EventBase* reconnectEvb_;
  folly::EventBase* subscriberEvb_;
  std::unique_ptr<FsdbPatchSubscriber> subscriber_;
  SubscriptionKey nextKey_{0};
  std::map<SubscriptionKey, RawOperPath> subscribePaths_;
  std::unique_ptr<FsdbExtPatchSubscriber> extSubscriber_;
  std::map<SubscriptionKey, ExtendedOperPath> extSubscribePaths_;

 private:
  // Delete copy operations
  FsdbSubManagerBase(const FsdbSubManagerBase&) = delete;
  FsdbSubManagerBase& operator=(const FsdbSubManagerBase&) = delete;

  // Delete move operations
  FsdbSubManagerBase(FsdbSubManagerBase&&) = delete;
  FsdbSubManagerBase& operator=(FsdbSubManagerBase&&) = delete;
};

} // namespace facebook::fboss::fsdb
