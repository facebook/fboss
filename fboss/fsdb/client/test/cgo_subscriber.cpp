// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/test/cgo_subscriber.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <chrono>

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/instantiations/FsdbCowStateSubManager.h"
#include "fboss/fsdb/client/instantiations/FsdbCowStatsSubManager.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

namespace facebook::fboss::fsdb {

namespace {

std::string serializeOperState(const OperState& state) {
  return apache::thrift::SimpleJSONSerializer::serialize<std::string>(state);
}

std::string serializeOperDelta(const OperDelta& delta) {
  return apache::thrift::SimpleJSONSerializer::serialize<std::string>(delta);
}

// Wrapper to allow using addPath() with runtime string paths for state
struct RawStatePath {
  using RootT = FsdbOperStateRoot;

  explicit RawStatePath(std::vector<std::string> tokens)
      : tokens_(std::move(tokens)) {}

  std::vector<std::string> idTokens() const {
    return tokens_;
  }

 private:
  std::vector<std::string> tokens_;
};

// Wrapper to allow using addPath() with runtime string paths for stats
struct RawStatsPath {
  using RootT = FsdbOperStatsRoot;

  explicit RawStatsPath(std::vector<std::string> tokens)
      : tokens_(std::move(tokens)) {}

  std::vector<std::string> idTokens() const {
    return tokens_;
  }

 private:
  std::vector<std::string> tokens_;
};

template <typename SubUpdate>
std::string serializeSubUpdate(const SubUpdate& update) {
  std::string result = "{";
  result += "\"updatedPaths\": [";
  bool first = true;
  for (const auto& path : update.updatedPaths) {
    if (!first) {
      result += ", ";
    }
    result += "\"/" + folly::join("/", path) + "\"";
    first = false;
  }
  result += "]";
  if (update.lastServedAt.has_value()) {
    result += ", \"lastServedAt\": " + std::to_string(*update.lastServedAt);
  }
  if (update.lastPublishedAt.has_value()) {
    result +=
        ", \"lastPublishedAt\": " + std::to_string(*update.lastPublishedAt);
  }
  result += "}";
  return result;
}

} // namespace

FsdbCgoSubscriber::FsdbCgoSubscriber(
    std::string clientId,
    std::string hostIp,
    int32_t fsdbPort) noexcept
    : clientId_(std::move(clientId)),
      hostIp_(std::move(hostIp)),
      fsdbPort_(fsdbPort) {
  pubSubManager_ = std::make_unique<FsdbPubSubManager>(clientId_);
}

FsdbCgoSubscriber::~FsdbCgoSubscriber() {
  cleanup();
}

void FsdbCgoSubscriber::init() {
  XLOG(INFO) << "FsdbCgoSubscriber::init() called for client: " << clientId_;
  // Placeholder for future test harness setup
}

void FsdbCgoSubscriber::cleanup() {
  unsubscribeAll();
  if (pubSubManager_) {
    pubSubManager_.reset();
  }
  XLOG(INFO) << "FsdbCgoSubscriber::cleanup() completed";
}

void FsdbCgoSubscriber::subscribeStatePath(std::vector<std::string> path) {
  if (!pathSubscriptionHandle_.empty()) {
    XLOG(WARNING)
        << "State path subscription already active, unsubscribing first";
    unsubscribeStatePath();
  }

  pathSubscribePath_ = path;

  auto stateChangeCb = [this](
                           SubscriptionState oldState,
                           SubscriptionState newState,
                           std::optional<bool> initialSyncHasData) {
    XLOG(DBG2) << "State path subscription state changed: "
               << static_cast<int>(oldState) << " -> "
               << static_cast<int>(newState);
    pathTracker_.state.store(static_cast<int32_t>(newState));
    if (initialSyncHasData.has_value()) {
      pathTracker_.initialSyncComplete.store(true);
    }
  };

  auto dataCallback = [this](OperState&& state) {
    XLOG(DBG3) << "Received state path update";
    pathTracker_.updateCount.fetch_add(1);
    pathTracker_.lastContent.wlock()->assign(serializeOperState(state));
    // Reset and post the baton to allow multiple updates
    updateBaton_.post();
  };

  pathSubscriptionHandle_ = pubSubManager_->addStatePathSubscription(
      path,
      std::move(stateChangeCb),
      std::move(dataCallback),
      utils::ConnectionOptions(hostIp_, fsdbPort_));
  XLOG(INFO) << "Subscribed to state path: " << folly::join("/", path);
}

void FsdbCgoSubscriber::subscribeStatPath(std::vector<std::string> path) {
  if (!pathSubscriptionHandle_.empty()) {
    XLOG(WARNING)
        << "Stat path subscription already active, unsubscribing first";
    unsubscribeStatPath();
  }

  pathSubscribePath_ = path;

  auto stateChangeCb = [this](
                           SubscriptionState oldState,
                           SubscriptionState newState,
                           std::optional<bool> initialSyncHasData) {
    XLOG(DBG2) << "Stat path subscription state changed: "
               << static_cast<int>(oldState) << " -> "
               << static_cast<int>(newState);
    pathTracker_.state.store(static_cast<int32_t>(newState));
    if (initialSyncHasData.has_value()) {
      pathTracker_.initialSyncComplete.store(true);
    }
  };

  auto dataCallback = [this](OperState&& state) {
    XLOG(DBG3) << "Received stat path update";
    pathTracker_.updateCount.fetch_add(1);
    pathTracker_.lastContent.wlock()->assign(serializeOperState(state));
    // Reset and post the baton to allow multiple updates
    updateBaton_.post();
  };

  pathSubscriptionHandle_ = pubSubManager_->addStatPathSubscription(
      path,
      std::move(stateChangeCb),
      std::move(dataCallback),
      utils::ConnectionOptions(hostIp_, fsdbPort_));
  XLOG(INFO) << "Subscribed to stat path: " << folly::join("/", path);
}

void FsdbCgoSubscriber::unsubscribeStatePath() {
  if (!pathSubscriptionHandle_.empty()) {
    pubSubManager_->removeStatePathSubscription(pathSubscribePath_, hostIp_);
    pathSubscriptionHandle_.clear();
    pathSubscribePath_.clear();
    pathTracker_.state.store(0);
    pathTracker_.initialSyncComplete.store(false);
    XLOG(INFO) << "Unsubscribed from state path";
  }
}

void FsdbCgoSubscriber::unsubscribeStatPath() {
  if (!pathSubscriptionHandle_.empty()) {
    pubSubManager_->removeStatPathSubscription(pathSubscribePath_, hostIp_);
    pathSubscriptionHandle_.clear();
    pathSubscribePath_.clear();
    pathTracker_.state.store(0);
    pathTracker_.initialSyncComplete.store(false);
    XLOG(INFO) << "Unsubscribed from stat path";
  }
}

void FsdbCgoSubscriber::subscribeStateDelta(std::vector<std::string> path) {
  if (!deltaSubscriptionHandle_.empty()) {
    XLOG(WARNING)
        << "State delta subscription already active, unsubscribing first";
    unsubscribeStateDelta();
  }

  deltaSubscribePath_ = path;

  auto stateChangeCb = [this](
                           SubscriptionState oldState,
                           SubscriptionState newState,
                           std::optional<bool> initialSyncHasData) {
    XLOG(DBG2) << "State delta subscription state changed: "
               << static_cast<int>(oldState) << " -> "
               << static_cast<int>(newState);
    deltaTracker_.state.store(static_cast<int32_t>(newState));
    if (initialSyncHasData.has_value()) {
      deltaTracker_.initialSyncComplete.store(true);
    }
  };

  auto dataCallback = [this](OperDelta&& delta) {
    XLOG(DBG3) << "Received state delta update";
    deltaTracker_.updateCount.fetch_add(1);
    deltaTracker_.lastContent.wlock()->assign(serializeOperDelta(delta));
    // Reset and post the baton to allow multiple updates
    updateBaton_.post();
  };

  deltaSubscriptionHandle_ = pubSubManager_->addStateDeltaSubscription(
      path,
      std::move(stateChangeCb),
      std::move(dataCallback),
      utils::ConnectionOptions(hostIp_, fsdbPort_));
  XLOG(INFO) << "Subscribed to state delta: " << folly::join("/", path);
}

void FsdbCgoSubscriber::subscribeStatDelta(std::vector<std::string> path) {
  if (!deltaSubscriptionHandle_.empty()) {
    XLOG(WARNING)
        << "Stat delta subscription already active, unsubscribing first";
    unsubscribeStatDelta();
  }

  deltaSubscribePath_ = path;

  auto stateChangeCb = [this](
                           SubscriptionState oldState,
                           SubscriptionState newState,
                           std::optional<bool> initialSyncHasData) {
    XLOG(DBG2) << "Stat delta subscription state changed: "
               << static_cast<int>(oldState) << " -> "
               << static_cast<int>(newState);
    deltaTracker_.state.store(static_cast<int32_t>(newState));
    if (initialSyncHasData.has_value()) {
      deltaTracker_.initialSyncComplete.store(true);
    }
  };

  auto dataCallback = [this](OperDelta&& delta) {
    XLOG(DBG3) << "Received stat delta update";
    deltaTracker_.updateCount.fetch_add(1);
    deltaTracker_.lastContent.wlock()->assign(serializeOperDelta(delta));
    // Reset and post the baton to allow multiple updates
    updateBaton_.post();
  };

  deltaSubscriptionHandle_ = pubSubManager_->addStateDeltaSubscription(
      path,
      std::move(stateChangeCb),
      std::move(dataCallback),
      utils::ConnectionOptions(hostIp_, fsdbPort_));
  XLOG(INFO) << "Subscribed to stat delta: " << folly::join("/", path);
}

void FsdbCgoSubscriber::unsubscribeStateDelta() {
  if (!deltaSubscriptionHandle_.empty()) {
    pubSubManager_->removeStateDeltaSubscription(deltaSubscribePath_, hostIp_);
    deltaSubscriptionHandle_.clear();
    deltaSubscribePath_.clear();
    deltaTracker_.state.store(0);
    deltaTracker_.initialSyncComplete.store(false);
    XLOG(INFO) << "Unsubscribed from state delta";
  }
}

void FsdbCgoSubscriber::unsubscribeStatDelta() {
  if (!deltaSubscriptionHandle_.empty()) {
    pubSubManager_->removeStateDeltaSubscription(deltaSubscribePath_, hostIp_);
    deltaSubscriptionHandle_.clear();
    deltaSubscribePath_.clear();
    deltaTracker_.state.store(0);
    deltaTracker_.initialSyncComplete.store(false);
    XLOG(INFO) << "Unsubscribed from stat delta";
  }
}

void FsdbCgoSubscriber::subscribeStatePatch(
    std::vector<std::vector<std::string>> paths,
    bool isStats) {
  if (patchSubscriptionActive_) {
    XLOG(WARNING) << "Patch subscription already active, unsubscribing first";
    unsubscribeStatePatch();
  }

  patchIsStats_ = isStats;

  SubscriptionOptions subscriptionOptions(clientId_, isStats);
  utils::ConnectionOptions connectionOptions(hostIp_, fsdbPort_);

  auto stateChangeCb = [this](
                           SubscriptionState oldState,
                           SubscriptionState newState,
                           std::optional<bool> initialSyncHasData) {
    XLOG(DBG2) << "Patch subscription state changed: "
               << static_cast<int>(oldState) << " -> "
               << static_cast<int>(newState);
    patchTracker_.state.store(static_cast<int32_t>(newState));
    if (initialSyncHasData.has_value()) {
      patchTracker_.initialSyncComplete.store(true);
    }
  };

  if (isStats) {
    auto* manager = new FsdbCowStatsSubManager(
        std::move(subscriptionOptions), std::move(connectionOptions));
    subManagerCleanup_ = [manager]() { delete manager; };

    for (auto& path : paths) {
      manager->addPath(RawStatsPath(path));
    }

    auto dataCallback = [this](FsdbCowStatsSubManager::SubUpdate update) {
      XLOG(DBG3) << "Received stats patch update";
      patchTracker_.updateCount.fetch_add(1);
      patchTracker_.lastContent.wlock()->assign(serializeSubUpdate(update));
      updateBaton_.post();
    };

    manager->subscribe(std::move(dataCallback), std::move(stateChangeCb));
  } else {
    auto* manager = new FsdbCowStateSubManager(
        std::move(subscriptionOptions), std::move(connectionOptions));
    subManagerCleanup_ = [manager]() { delete manager; };

    for (auto& path : paths) {
      manager->addPath(RawStatePath(path));
    }

    auto dataCallback = [this](FsdbCowStateSubManager::SubUpdate update) {
      XLOG(DBG3) << "Received state patch update";
      patchTracker_.updateCount.fetch_add(1);
      patchTracker_.lastContent.wlock()->assign(serializeSubUpdate(update));
      updateBaton_.post();
    };

    manager->subscribe(std::move(dataCallback), std::move(stateChangeCb));
  }

  patchSubscriptionActive_ = true;
  XLOG(INFO) << "Subscribed to " << (isStats ? "stats" : "state")
             << " patch: " << paths.size() << " path(s)";
}

void FsdbCgoSubscriber::unsubscribeStatePatch() {
  if (patchSubscriptionActive_) {
    if (subManagerCleanup_) {
      subManagerCleanup_();
      subManagerCleanup_ = nullptr;
    }
    patchSubscriptionActive_ = false;
    patchIsStats_ = false;
    patchTracker_.state.store(0);
    patchTracker_.initialSyncComplete.store(false);
    XLOG(INFO) << "Unsubscribed from patch";
  }
}

void FsdbCgoSubscriber::unsubscribeAll() {
  unsubscribeStatePath();
  unsubscribeStatPath();
  unsubscribeStateDelta();
  unsubscribeStatDelta();
  unsubscribeStatePatch();
}

int32_t FsdbCgoSubscriber::getSubscriptionState(
    int32_t subscriptionType) const {
  switch (subscriptionType) {
    case 0:
      return pathTracker_.state.load();
    case 1:
      return deltaTracker_.state.load();
    case 2:
      return patchTracker_.state.load();
    default:
      XLOG(WARNING) << "Invalid subscription type: " << subscriptionType;
      return 0;
  }
}

bool FsdbCgoSubscriber::isInitialSyncComplete(int32_t subscriptionType) const {
  switch (subscriptionType) {
    case 0:
      return pathTracker_.initialSyncComplete.load();
    case 1:
      return deltaTracker_.initialSyncComplete.load();
    case 2:
      return patchTracker_.initialSyncComplete.load();
    default:
      XLOG(WARNING) << "Invalid subscription type: " << subscriptionType;
      return false;
  }
}

bool FsdbCgoSubscriber::waitForUpdate(int64_t timeoutMs) {
  if (timeoutMs < 0) {
    updateBaton_.wait();
    updateBaton_.reset();
    return true;
  }
  bool received =
      updateBaton_.try_wait_for(std::chrono::milliseconds(timeoutMs));
  // reset() MUST be called after every try_wait_for() before the baton can
  // be reused, whether the wait timed out or not (see folly::Baton docs).
  updateBaton_.reset();
  return received;
}

int64_t FsdbCgoSubscriber::getPathUpdateCount() const {
  return pathTracker_.updateCount.load();
}

int64_t FsdbCgoSubscriber::getDeltaUpdateCount() const {
  return deltaTracker_.updateCount.load();
}

int64_t FsdbCgoSubscriber::getPatchUpdateCount() const {
  return patchTracker_.updateCount.load();
}

std::string FsdbCgoSubscriber::getLastPathUpdateContent() const {
  return pathTracker_.lastContent.copy();
}

std::string FsdbCgoSubscriber::getLastDeltaUpdateContent() const {
  return deltaTracker_.lastContent.copy();
}

std::string FsdbCgoSubscriber::getLastPatchUpdateContent() const {
  return patchTracker_.lastContent.copy();
}

} // namespace facebook::fboss::fsdb
