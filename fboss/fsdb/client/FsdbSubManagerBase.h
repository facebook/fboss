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

  void stop();

  const std::string& clientId() const;

  std::optional<SubscriptionInfo> getInfo();

 protected:
  FsdbSubManagerBase(
      fsdb::SubscriptionOptions opts,
      utils::ConnectionOptions serverOptions,
      folly::EventBase* reconnectEvb,
      folly::EventBase* subscriberEvb);

  SubscriptionKey addPathImpl(const std::vector<std::string>& pathTokens);

  SubscriptionKey addExtendedPathImpl(
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
