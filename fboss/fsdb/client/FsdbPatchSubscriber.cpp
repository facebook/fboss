// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPatchSubscriber.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

#include <folly/coro/Task.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss::fsdb {

namespace {
constexpr std::chrono::seconds kLiveAddRpcTimeout{5};
} // namespace

template <typename MessageType, typename SubUnit, typename PathElement>
SubRequest
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::createRequest(
    const PathElement& addedPaths) const {
  SubRequest request;
  request.clientId()->instanceId() = clientId();
  request.forceSubscribe() = this->subscriptionOptions().forceSubscribe_;
  if (this->subscriptionOptions().heartbeatInterval_.has_value()) {
    request.heartbeatInterval() =
        this->subscriptionOptions().heartbeatInterval_.value();
  }
  if (this->subscriptionOptions().serveIntervalSec_.has_value()) {
    request.serveIntervalSec() =
        this->subscriptionOptions().serveIntervalSec_.value();
  }
  // Merge the original subscribe paths with any paths appended post-subscribe
  // so a (re)connect re-subscribes the full set.
  PathElement merged = this->subscribePaths();
  for (const auto& [key, path] : addedPaths) {
    merged.insert_or_assign(key, path);
  }
  if constexpr (std::is_same_v<
                    PathElement,
                    std::map<SubscriptionKey, ExtendedOperPath>>) {
    request.extPaths() = std::move(merged);
  } else {
    request.paths() = std::move(merged);
  }
  return request;
}

template <typename MessageType, typename SubUnit, typename PathElement>
bool FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::
    claimLiveAddDrain(LiveAddState& state) {
  if (!state.uid.has_value() || state.pendingKeys.empty() ||
      state.drainScheduled) {
    return false;
  }
  state.drainScheduled = true;
  return true;
}

template <typename MessageType, typename SubUnit, typename PathElement>
std::optional<FsdbErrorCode>
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::addPaths(
    const PathElement& newPaths) {
  if (newPaths.empty()) {
    return std::nullopt;
  }
  // Recorded paths are delivered by the createRequest() merge on any
  // (re)connect, independent of the live add.
  bool schedule = liveAdd_.withWLock([&newPaths](auto& state) {
    for (const auto& [key, path] : newPaths) {
      state.addedPaths.insert_or_assign(key, path);
      state.pendingKeys.insert(key);
    }
    return claimLiveAddDrain(state);
  });
  if (schedule) {
    scheduleLiveAddDrain();
  }
  return std::nullopt;
}

template <typename MessageType, typename SubUnit, typename PathElement>
void FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::
    scheduleLiveAddDrain() {
  if (this->isCancelled()) {
    liveAdd_.wlock()->drainScheduled = false;
    return;
  }
  // On the stream EventBase, serialized with setupStream() and client_ resets;
  // on serviceLoopScope_ so teardown cancels and joins it.
  this->serviceLoopScope_.add(
      co_withExecutor(this->getStreamEventBase(), drainLiveAdds()));
}

template <typename MessageType, typename SubUnit, typename PathElement>
AddPatchSubscriptionPathsRequest
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::makeLiveAddRequest(
    uint64_t uid,
    const PathElement& paths) const {
  AddPatchSubscriptionPathsRequest request;
  request.clientId()->instanceId() = this->clientId();
  request.subscriptionUid() = uid;
  // Round-trip token: the server echoes the largest (most recently added) key
  // back as OperMetadata.streamRevision once the add takes effect.
  request.lastStreamRevision() = paths.rbegin()->first;
  if constexpr (std::is_same_v<
                    PathElement,
                    std::map<SubscriptionKey, ExtendedOperPath>>) {
    request.extPaths() = paths;
  } else {
    request.paths() = paths;
  }
  return request;
}

template <typename MessageType, typename SubUnit, typename PathElement>
void FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::
    eraseLiveAddKeys(uint64_t uid, const PathElement& batch) {
  liveAdd_.withWLock([&](auto& state) {
    // After a reconnect, pendingKeys is relative to the new request.
    if (state.uid == uid) {
      for (const auto& [key, _] : batch) {
        state.pendingKeys.erase(key);
      }
    }
  });
}

template <typename MessageType, typename SubUnit, typename PathElement>
folly::coro::Task<void>
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::liveAddRpc(
    AddPatchSubscriptionPathsRequest request,
    std::chrono::milliseconds timeout) {
  if (!this->client_) {
    throw std::runtime_error("no FSDB client");
  }
  // Unary RPC needs its own request timeout; getRpcOptions() only carries a
  // streaming chunk timeout.
  apache::thrift::RpcOptions rpcOptions;
  rpcOptions.setTimeout(timeout);
  auto task = this->isStats()
      ? this->client_->co_addStatsPatchSubscriptionPaths(rpcOptions, request)
      : this->client_->co_addStatePatchSubscriptionPaths(rpcOptions, request);
  co_await std::move(task);
}

template <typename MessageType, typename SubUnit, typename PathElement>
folly::coro::Task<void>
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::sendLiveAddBatch(
    uint64_t uid,
    const PathElement& batch) {
  try {
    co_await liveAddRpc(makeLiveAddRequest(uid, batch), kLiveAddRpcTimeout);
  } catch (const folly::OperationCancelled&) {
    throw;
  } catch (const std::exception& e) {
    XLOG(WARN) << "addPatchSubscriptionPaths live-extend failed for "
               << this->clientId() << ": " << folly::exceptionStr(e)
               << "; added paths will be delivered on next reconnect";
  }
}

template <typename MessageType, typename SubUnit, typename PathElement>
folly::coro::Task<void>
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::drainLiveAdds() {
  try {
    while (true) {
      uint64_t uid{0};
      PathElement batch;
      bool done = liveAdd_.withWLock([&](auto& state) {
        if (!state.uid.has_value() || state.pendingKeys.empty() ||
            this->isCancelled() || !this->client_) {
          state.drainScheduled = false;
          return true;
        }
        uid = *state.uid;
        for (const auto key : state.pendingKeys) {
          batch.emplace(key, state.addedPaths.at(key));
        }
        return false;
      });
      if (done) {
        co_return;
      }
      co_await sendLiveAddBatch(uid, batch);
      eraseLiveAddKeys(uid, batch);
    }
  } catch (const folly::OperationCancelled&) {
    liveAdd_.wlock()->drainScheduled = false;
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Live add-paths drain failed for " << this->clientId() << ": "
              << folly::exceptionStr(ex);
    liveAdd_.wlock()->drainScheduled = false;
  }
}

template <typename MessageType, typename SubUnit, typename PathElement>
folly::coro::Task<
    typename FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::
        StreamT>
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::setupStream() {
  // Build the request and reset per-stream live-add state atomically: paths
  // added after this point are missing from the request and are applied live
  // once this stream's uid is known. Keeping the request alive here also works
  // around a GCC coroutine bug (materialize the Task before co_await).
  auto request = liveAdd_.withWLock([this](auto& state) {
    state.uid.reset();
    state.pendingKeys.clear();
    return createRequest(state.addedPaths);
  });
  if (auto hook = onSubscribeRequestCreatedForTest_.copy()) {
    hook();
  }
  auto initResponseReceiver =
      [this](const OperSubInitResponse& initResponse) -> bool {
    // The server mints a new uid per stream; capture it so live adds target
    // this stream, and apply paths the request missed.
    if (initResponse.subscriptionUid().has_value()) {
      bool schedule = liveAdd_.withWLock([&initResponse](auto& state) {
        state.uid = *initResponse.subscriptionUid();
        return claimLiveAddDrain(state);
      });
      if (schedule) {
        scheduleLiveAddDrain();
      }
    }
    return !this->isCancelled();
  };

  if constexpr (std::is_same_v<
                    PathElement,
                    std::map<SubscriptionKey, ExtendedOperPath>>) {
    auto task = this->isStats() ? this->client_->co_subscribeStatsExtended(
                                      this->getRpcOptions(), request)
                                : this->client_->co_subscribeStateExtended(
                                      this->getRpcOptions(), request);
    auto result = co_await std::move(task);
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  } else {
    auto task = this->isStats()
        ? this->client_->co_subscribeStats(this->getRpcOptions(), request)
        : this->client_->co_subscribeState(this->getRpcOptions(), request);
    auto result = co_await std::move(task);
    initResponseReceiver(result.response);
    co_return std::move(result.stream);
  }
}

std::optional<OperMetadata> getChunkMetadata(const SubscriberChunk& chunk) {
  std::optional<OperMetadata> metadata;
  if (chunk.patchGroups()->size() > 0) {
    for (const auto& [key, patchGroup] : *chunk.patchGroups()) {
      if (!patchGroup.empty()) {
        const auto& patch = patchGroup.front();
        metadata = *patch.metadata();
      }
    }
  }
  return metadata;
}

uint64_t getChunkMetadataPublishTime(const SubscriberChunk& chunk) {
  auto md = getChunkMetadata(chunk);
  if (md.has_value()) {
    return md->lastPublishedAt().value_or(0);
  }
  return 0;
}

template <typename MessageType, typename SubUnit, typename PathElement>
folly::coro::Task<void>
FsdbPatchSubscriberImpl<MessageType, SubUnit, PathElement>::serveStream(
    StreamT&& stream) {
  CHECK(std::holds_alternative<SubStreamT>(stream));
  // add histogram for publish-subscribe time latency
  // histogram range [0, 20s], 200ms width (100 bins)
  fb303::ThreadCachedServiceData::get()->addHistogram(
      this->clientPubsubLatencyMetric_, 200, 0, 2000);
  fb303::ThreadCachedServiceData::get()->exportHistogram(
      this->clientPubsubLatencyMetric_, 50, 95, 99);
  if (this->subscriptionOptions().exportPerSubscriptionMetrics_) {
    fb303::ThreadCachedServiceData::get()->addHistogram(
        this->subscribeLatencyMetric_, 200, 0, 2000);
    fb303::ThreadCachedServiceData::get()->exportHistogram(
        this->subscribeLatencyMetric_, 50, 95, 99);
  }
  auto gen = std::move(std::get<SubStreamT>(stream)).toAsyncGenerator();
  while (auto message = co_await gen.next()) {
    if (this->isCancelled()) {
      XLOG(DBG2) << " Detected cancellation: " << this->clientId();
      break;
    }

    bool hasData = (message->getType() == SubscriberMessage::Type::chunk);
    bool isInitialSync =
        (BaseT::getSubscriptionState() != SubscriptionState::CONNECTED);

    if (!isInitialSync && hasData) {
      // compute publish-subscribe time latency
      auto publishTime = getChunkMetadataPublishTime(message->get_chunk());
      if (publishTime > 0) {
        auto currentTimestamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        auto latency = currentTimestamp - publishTime;
        fb303::ThreadCachedServiceData::get()->addHistogramValue(
            this->clientPubsubLatencyMetric_, latency);
        if (this->subscriptionOptions().exportPerSubscriptionMetrics_) {
          fb303::ThreadCachedServiceData::get()->addHistogramValue(
              this->subscribeLatencyMetric_, latency);
        }
      }
    }

    if (!this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
        this->getSubscriptionState() != SubscriptionState::CONNECTED) {
      BaseT::updateSubscriptionState(SubscriptionState::CONNECTED, hasData);
    }
    switch (message->getType()) {
      case SubscriberMessage::Type::chunk:
        if (this->subscriptionOptions().requireInitialSyncToMarkConnect_ &&
            this->getSubscriptionState() != SubscriptionState::CONNECTED) {
          BaseT::updateSubscriptionState(SubscriptionState::CONNECTED, hasData);
        }
        this->onChunkReceived(false, getChunkMetadata(message->get_chunk()));
        try {
          this->operSubUnitUpdate_(message->move_chunk());
        } catch (const std::exception& ex) {
          FsdbException e;
          e.message() = folly::exceptionStr(ex);
          e.errorCode() = FsdbErrorCode::SUBSCRIPTION_DATA_CALLBACK_ERROR;
          throw e;
        }
        break;
      case SubscriberMessage::Type::heartbeat:
        if (message->get_heartbeat().metadata().has_value()) {
          this->onChunkReceived(
              true, message->get_heartbeat().metadata().value());
        } else {
          this->onChunkReceived(true, std::nullopt);
        }
        break;
      case SubscriberMessage::Type::__EMPTY__:
        break;
    }
  }
  co_return;
}

template class FsdbPatchSubscriberImpl<
    SubscriberMessage,
    SubscriberChunk,
    std::map<SubscriptionKey, RawOperPath>>;

template class FsdbPatchSubscriberImpl<
    SubscriberMessage,
    SubscriberChunk,
    std::map<SubscriptionKey, ExtendedOperPath>>;

} // namespace facebook::fboss::fsdb
