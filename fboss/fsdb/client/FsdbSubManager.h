// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/fsdb/client/FsdbPatchSubscriber.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/io/async/ScopedEventBaseThread.h>

namespace facebook::fboss::fsdb {

auto constexpr kReconnectThread = "FsdbReconnectThread";
auto constexpr kSubscriberThread = "FsdbSubscriberThread";

/*
 * Subscription Manager for FSDB that handles multiple paths and uses the most
 * efficient transport protocol available (currently Patch). This is effectively
 * a wrapper around the stream client that handles the difficult to use encoded
 * patches and exposes a parsed object to the callback.
 *
 * To use, create the object, add paths, then call subscribe with a callback to
 * initiate the stream.
 *
 * NOTE: This class should not be included directly, instead use one of the
 * instantiations, depending on which data type you want
 */
template <typename _Storage, bool _IsCow>
class FsdbSubManager {
 public:
  using Storage = _Storage;
  using Root = typename Storage::RootT;
  using Data = std::shared_ptr<typename Storage::StorageImpl>;

  static_assert(
      std::is_same_v<Root, FsdbOperStateRoot> ||
      std::is_same_v<Root, FsdbOperStatsRoot>);
  static constexpr bool IsStats = std::is_same_v<Root, FsdbOperStatsRoot>;
  static constexpr bool IsCow = _IsCow;
  static_assert(IsCow, "Do not support raw thrift yet");

  /*
   * Callback for state updates.
   * Callback will always received the FSDB root in the form of raw thrift or
   * thrift_cow object, depending on type of subscriber. Caller will use the
   * changedKeys or changedPaths params to determine what changed and pull the
   * respective data out of the root. changedKeys can be compared against the
   * SubscriptionKey returned from addPath.
   */
  struct SubUpdate {
    // Data will always received the FSDB root in the form of raw thrift or
    // thrift_cow object, depending on type of subscriber
    const Data data;
    // SubscriptionKeys that changed, can be compared against keys returned from
    // addPath
    std::vector<SubscriptionKey> updatedKeys;
    // concrete paths that changed
    std::vector<std::vector<std::string>> updatedPaths;
  };

  using DataCallback = std::function<void(SubUpdate)>;

  FsdbSubManager(
      fsdb::SubscriptionOptions opts,
      ReconnectingThriftClient::ServerOptions serverOptions,
      folly::EventBase* reconnectEvb = nullptr,
      folly::EventBase* subscriberEvb = nullptr)
      : opts_(std::move(opts)),
        serverOptions_(std::move(serverOptions)),
        reconnectEvbThread_(
            reconnectEvb ? nullptr
                         : std::make_unique<folly::ScopedEventBaseThread>(
                               kReconnectThread)),
        subscriberEvbThread_(
            subscriberEvb ? nullptr
                          : std::make_unique<folly::ScopedEventBaseThread>(
                                kSubscriberThread)),
        // if local thread available, use local evb otherwise use passed in
        reconnectEvb_(
            reconnectEvbThread_ ? reconnectEvbThread_->getEventBase()
                                : reconnectEvb),
        subscriberEvb_(
            subscriberEvbThread_ ? subscriberEvbThread_->getEventBase()
                                 : subscriberEvb) {
    CHECK_EQ(IsStats, opts_.subscribeStats_);
  }

  ~FsdbSubManager() {
    stop();
  }

  /*
   * Takes a thriftpath object. Return a SubscriptionKey that can be used in the
   * callback to efficiently check what data changed.
   * Must be called before subscribe.
   */
  template <typename Path>
  SubscriptionKey addPath(Path path) {
    static_assert(
        std::is_same_v<typename Path::RootT, Root>,
        "Path root must be same as Root type");
    CHECK(!subscriber_) << "Cannot add paths after subscribed";
    // TODO: verify paths are not cross root and are not ancestors/descendants
    auto key = nextKey_++;
    RawOperPath p;
    p.path() = path.idTokens();
    auto res = subscribePaths_.insert_or_assign(key, std::move(p));
    CHECK(res.second) << "Duplicate path added";
    return key;
  }

  /*
   * Initiate subscription. Must be called after all interested paths are added.
   * See DataCallback for callback signature.
   */
  void subscribe(
      DataCallback cb,
      std::optional<SubscriptionStateChangeCb> subscriptionStateChangeCb =
          std::nullopt) {
    CHECK(!subscriber_) << "Cannot subscribe twice";
    subscriber_ = std::make_unique<FsdbPatchSubscriber>(
        SubscriptionOptions(opts_),
        subscribePaths_,
        subscriberEvb_,
        reconnectEvb_,
        [this, cb = std::move(cb)](SubscriberChunk chunk) {
          parseChunkAndInvokeCallback(std::move(chunk), std::move(cb));
        },
        std::move(subscriptionStateChangeCb));
    subscriber_->setServerOptions(
        ReconnectingThriftClient::ServerOptions(serverOptions_));
  }

  // Returns a synchronized data object that is always kept up to date
  // with FSDB data
  folly::Synchronized<Data> subscribeBound(
      std::optional<SubscriptionStateChangeCb> subscriptionStateChangeCb =
          std::nullopt) {
    folly::Synchronized<Data> boundData;
    subscribe(
        [&](SubUpdate update) { *boundData.wlock() = update.data; },
        std::move(subscriptionStateChangeCb));
    return boundData;
  }

  void stop() {
    subscriber_.reset();
  }

  const std::string& clientId() const {
    return opts_.clientId_;
  }

  std::optional<SubscriptionInfo> getInfo() {
    if (subscriber_) {
      return subscriber_->getInfo();
    }
    return std::nullopt;
  }

 private:
  void parseChunkAndInvokeCallback(SubscriberChunk chunk, DataCallback cb) {
    std::vector<SubscriptionKey> changedKeys;
    changedKeys.reserve(chunk.patchGroups()->size());
    std::vector<std::vector<std::string>> changedPaths;
    changedPaths.reserve(chunk.patchGroups()->size());
    for (auto& [key, patchGroup] : *chunk.patchGroups()) {
      // FsdbSubManager only supports non-wildcard subs, which will always have
      // a single patch per path
      CHECK_EQ(patchGroup.size(), 1);
      auto& patch = patchGroup.front();
      changedKeys.push_back(key);
      changedPaths.emplace_back(*patch.basePath());
      // TODO: support patching a raw thrift object
      root_.patch(std::move(patch));
    }
    root_.publish();
    SubUpdate update{
        root_.root(), std::move(changedKeys), std::move(changedPaths)};
    cb(std::move(update));
  }

  fsdb::SubscriptionOptions opts_;
  ReconnectingThriftClient::ServerOptions serverOptions_;

  // local threads are only needed when there are no external eventbases
  std::unique_ptr<folly::ScopedEventBaseThread> reconnectEvbThread_{nullptr};
  std::unique_ptr<folly::ScopedEventBaseThread> subscriberEvbThread_{nullptr};

  // eventbase pointers from either external/internal threads
  folly::EventBase* reconnectEvb_;
  folly::EventBase* subscriberEvb_;

  std::unique_ptr<FsdbPatchSubscriber> subscriber_;
  SubscriptionKey nextKey_{0};
  std::map<SubscriptionKey, RawOperPath> subscribePaths_;
  Storage root_{Root()};
};

} // namespace facebook::fboss::fsdb
