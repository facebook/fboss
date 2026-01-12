// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/oper/PathConverter.h>
#include "fboss/fsdb/client/FsdbSubManagerBase.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

namespace facebook::fboss::fsdb {

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
class FsdbSubManager : public FsdbSubManagerBase {
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
    // Minimum lastServedAt timestamp in milliseconds across all paths
    // (optional, only if metadata available)
    std::optional<int64_t> lastServedAt;
    // Maximum lastPublishedAt timestamps in milliseconds across all paths
    // (optional, only if metadata available)
    std::optional<int64_t> lastPublishedAt;
  };

  using DataCallback = std::function<void(SubUpdate)>;

  explicit FsdbSubManager(
      fsdb::SubscriptionOptions opts,
      utils::ConnectionOptions serverOptions =
          utils::ConnectionOptions("::1", FLAGS_fsdbPort),
      folly::EventBase* reconnectEvb = nullptr,
      folly::EventBase* subscriberEvb = nullptr);

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
    return addPathImpl(path.idTokens());
  }

  /*
   * Takes a ExtendedOperPath, and returns a SubscriptionKey that can be used
   * in the callback to efficiently check what data changed.
   * Must be called before subscribe.
   */
  template <typename ExtendedOperPath>
  SubscriptionKey addExtendedPath(ExtendedOperPath path) {
    return addExtendedPathImpl(
        PathConverter<Root>::extPathToIdTokens(*path.path()));
  }

  /*
   * Initiate subscription. Must be called after all interested paths are added.
   * See DataCallback for callback signature.
   */
  void subscribe(
      DataCallback cb,
      std::optional<SubscriptionStateChangeCb> subscriptionStateChangeCb =
          std::nullopt,
      std::optional<FsdbStreamHeartbeatCb> heartbeatCb = std::nullopt);

  // Returns a synchronized data object that is always kept up to date
  // with FSDB data
  folly::Synchronized<Data> subscribeBound(
      std::optional<SubscriptionStateChangeCb> subscriptionStateChangeCb =
          std::nullopt);

  void stop() override {
    FsdbSubManagerBase::stop();
    dataCb_.reset();
  }

 private:
  void parseChunkAndInvokeCallback(SubscriberChunk chunk);

  Storage root_{Root()};
  std::optional<DataCallback> dataCb_{std::nullopt};
};

} // namespace facebook::fboss::fsdb

#include "fboss/fsdb/client/FsdbSubManager-inl.h" // IWYU pragma: export
