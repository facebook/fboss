// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <fboss/fsdb/oper/CowSubscriptionManager.h>
#include <fboss/fsdb/oper/DeltaValue.h>
#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorageBase.h>
#include <fboss/fsdb/oper/PathConverter.h>
#include <fboss/fsdb/oper/SubscribableStorage.h>
#include <fboss/thrift_cow/storage/CowStorage.h>
#include <fboss/thrift_cow/storage/Storage.h>

#include <folly/Expected.h>
#include <folly/coro/Sleep.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <chrono>
#include <utility>

namespace facebook::fboss::fsdb {

template <typename Storage, typename SubscribeManager>
class NaivePeriodicSubscribableStorage
    : public NaivePeriodicSubscribableStorageBase,
      public SubscribableStorage<
          typename Storage::RootT,
          NaivePeriodicSubscribableStorage<Storage, SubscribeManager>> {
 public:
  // TODO: more flexibility here for all forward iterator types
  using RootT = typename Storage::RootT;
  using ConcretePath = typename Storage::ConcretePath;
  using PathIter = typename Storage::PathIter;
  using ExtPath = typename Storage::ExtPath;
  using ExtPathIter = typename Storage::ExtPathIter;
  using RootNode = typename Storage::StorageImpl;

  template <typename T>
  using Result = typename Storage::template Result<T>;

  using Self = NaivePeriodicSubscribableStorage<Storage, SubscribeManager>;
  using Base = SubscribableStorage<RootT, Self>;

  explicit NaivePeriodicSubscribableStorage(
      const RootT& initialState,
      StorageParams params = {});

  ~NaivePeriodicSubscribableStorage();

  using NaivePeriodicSubscribableStorageBase::start_impl;
  using NaivePeriodicSubscribableStorageBase::stop_impl;

  using Base::get;
  using Base::get_encoded;
  using Base::get_encoded_extended;
  using Base::remove;
  using Base::set;
  using Base::set_encoded;
  using Base::start;
  using Base::stop;
  using Base::subscribe;
  using Base::subscribe_delta;
  using Base::subscribe_delta_extended;
  using Base::subscribe_encoded;
  using Base::subscribe_encoded_extended;
  using Base::subscribe_patch;

  template <typename T>
  Result<T> get_impl(PathIter begin, PathIter end) const {
    if (params_.serveGetRequestsWithLastPublishedState_) {
      auto state = Storage(*lastPublishedState_.rlock());
      return state.template get<T>(begin, end);
    } else {
      // hold rlock on current state to avoid racing with writers
      auto currentState = currentState_.rlock();
      return currentState->template get<T>(begin, end);
    }
  }

  Result<OperState>
  get_encoded_impl(PathIter begin, PathIter end, OperProtocol protocol) const;

  Result<std::vector<TaggedOperState>> get_encoded_extended_impl(
      ExtPathIter begin,
      ExtPathIter end,
      OperProtocol protocol) const;

  template <typename T>
  std::optional<StorageError>
  set_impl(PathIter begin, PathIter end, T&& value) {
    auto state = currentState_.wlock();
    updateMetadata(begin, end);
    return state->set(begin, end, std::forward<T>(value));
  }

  std::optional<StorageError>
  set_encoded_impl(PathIter begin, PathIter end, const OperState& value);

  template <typename T>
  std::optional<StorageError>
  add_impl(PathIter begin, PathIter end, T&& value) {
    auto state = currentState_.wlock();
    updateMetadata(begin, end);
    return state->add(begin, end, std::forward<T>(value));
  }

  void remove_impl(PathIter begin, PathIter end);

  std::optional<StorageError> patch_impl(Patch&& patch);
  using NaivePeriodicSubscribableStorageBase::add_patch_subscription_path_impl;
  using NaivePeriodicSubscribableStorageBase::subscribe_patch_extended_impl;
  using NaivePeriodicSubscribableStorageBase::subscribe_patch_impl;

  std::optional<StorageError> patch_impl(const fsdb::OperDelta& delta);

  std::optional<StorageError> patch_impl(
      const fsdb::TaggedOperState& operState);

  using NaivePeriodicSubscribableStorageBase::subscribe_delta_extended_impl;
  using NaivePeriodicSubscribableStorageBase::subscribe_delta_impl;
  using NaivePeriodicSubscribableStorageBase::subscribe_encoded_extended_impl;
  using NaivePeriodicSubscribableStorageBase::subscribe_encoded_impl;
  using NaivePeriodicSubscribableStorageBase::subscribe_impl;

  std::tuple<
      std::shared_ptr<RootNode>,
      std::shared_ptr<RootNode>,
      SubscriptionMetadataServer>
  publishCurrentState();

  folly::coro::Task<void> serveSubscriptions() override;

  using NaivePeriodicSubscribableStorageBase::getSubscriptions;
  using NaivePeriodicSubscribableStorageBase::numPathStores;
  // Do not use, except for UTs that cross check numPathStores()
  using NaivePeriodicSubscribableStorageBase::numPathStoresRecursive_Expensive;
  using NaivePeriodicSubscribableStorageBase::numSubscriptions;
  using NaivePeriodicSubscribableStorageBase::setConvertToIDPaths;

  /*
   * Expensive API to copy current root. To be used only
   * in tests
   */
  RootT currentStateExpensive() const;

  OperState publishedStateEncoded(OperProtocol protocol);

 protected:
  const SubscriptionManagerBase& subMgr() const override;

  SubscriptionManagerBase& subMgr() override;

  ConcretePath convertPath(ConcretePath&& path) const override;

  ExtPath convertPath(const ExtPath& path) const override;

  folly::Synchronized<Storage> currentState_;
  folly::Synchronized<Storage> lastPublishedState_;

  SubscribeManager subscriptions_;
};

template <typename Root, bool EnableHybridStorage = false>
using NaivePeriodicSubscribableCowStorage = NaivePeriodicSubscribableStorage<
    CowStorage<
        Root,
        thrift_cow::ThriftStructNode<
            Root,
            thrift_cow::ThriftStructResolver<Root, EnableHybridStorage>,
            EnableHybridStorage>>,
    CowSubscriptionManager<thrift_cow::ThriftStructNode<
        Root,
        thrift_cow::ThriftStructResolver<Root, EnableHybridStorage>,
        EnableHybridStorage>>>;
} // namespace facebook::fboss::fsdb

#ifndef FBOSS_NAIVE_PERIODIC_SUBSCRIBABLE_STORAGE_DECLARATIONS_ONLY
#include <fboss/fsdb/oper/NaivePeriodicSubscribableStorage-inl.h>
#endif
