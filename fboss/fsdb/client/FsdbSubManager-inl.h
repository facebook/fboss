// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <glog/logging.h>
#include <chrono>

#include <folly/logging/xlog.h>

#include "fboss/fsdb/client/FsdbSubManager.h"

namespace facebook::fboss::fsdb {

template <typename _Storage, bool _IsCow>
FsdbSubManager<_Storage, _IsCow>::FsdbSubManager(
    fsdb::SubscriptionOptions opts,
    utils::ConnectionOptions serverOptions,
    folly::EventBase* reconnectEvb,
    folly::EventBase* subscriberEvb)
    : FsdbSubManagerBase(
          std::move(opts),
          std::move(serverOptions),
          reconnectEvb,
          subscriberEvb) {
  CHECK_EQ(IsStats, opts_.subscribeStats_);
}

template <typename _Storage, bool _IsCow>
void FsdbSubManager<_Storage, _IsCow>::subscribe(
    DataCallback cb,
    std::optional<SubscriptionStateChangeCb> subscriptionStateChangeCb,
    std::optional<FsdbStreamHeartbeatCb> heartbeatCb) {
  dataCb_ = std::move(cb);
  subscribeImpl(
      [this](SubscriberChunk chunk) {
        parseChunkAndInvokeCallback(std::move(chunk));
      },
      std::move(subscriptionStateChangeCb),
      std::move(heartbeatCb));
}

template <typename _Storage, bool _IsCow>
folly::Synchronized<typename FsdbSubManager<_Storage, _IsCow>::Data>
FsdbSubManager<_Storage, _IsCow>::subscribeBound(
    std::optional<SubscriptionStateChangeCb> subscriptionStateChangeCb) {
  folly::Synchronized<Data> boundData;
  subscribe(
      [&](SubUpdate update) { *boundData.wlock() = update.data; },
      std::move(subscriptionStateChangeCb));
  return boundData;
}

template <typename _Storage, bool _IsCow>
void FsdbSubManager<_Storage, _IsCow>::parseChunkAndInvokeCallback(
    SubscriberChunk chunk) {
  std::vector<SubscriptionKey> changedKeys;
  changedKeys.reserve(chunk.patchGroups()->size());
  std::vector<std::vector<std::string>> changedPaths;
  changedPaths.reserve(chunk.patchGroups()->size());
  std::optional<int64_t> lastServedAt;
  std::optional<int64_t> lastPublishedAt;

  for (auto& [key, patchGroup] : *chunk.patchGroups()) {
    for (auto& patch : patchGroup) {
      // Calculate served delay and track minimum lastServedAt timestamp
      const auto& metadata = *patch.metadata();

      if (metadata.lastServedAt().has_value()) {
        auto patchLastServedAt = *metadata.lastServedAt();
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
        auto delay = now - patchLastServedAt;

        XLOG(DBG2) << "FsdbSubManager subscription delay: " << delay
                   << " ms for path: " << folly::join("/", *patch.basePath());

        if (!lastServedAt.has_value() || patchLastServedAt < *lastServedAt) {
          lastServedAt = patchLastServedAt;
        }
      }
      if (metadata.lastPublishedAt().has_value()) {
        auto patchLastPublishedAt = *metadata.lastPublishedAt();
        if (!lastPublishedAt.has_value() ||
            patchLastPublishedAt > *lastPublishedAt) {
          lastPublishedAt = patchLastPublishedAt;
        }
      }
      changedKeys.push_back(key);
      changedPaths.emplace_back(*patch.basePath());
      root_.patch(std::move(patch));
    }
  }
  root_.publish();
  SubUpdate update{
      root_.root(),
      std::move(changedKeys),
      std::move(changedPaths),
      lastServedAt,
      lastPublishedAt};
  dataCb_.value()(std::move(update));
}

} // namespace facebook::fboss::fsdb
