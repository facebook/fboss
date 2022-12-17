// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <string>
#include "fboss/fsdb/client/FsdbDeltaPublisher.h"
#include "fboss/fsdb/client/FsdbStatePublisher.h"

namespace {
using namespace facebook::fboss::fsdb;
auto constexpr kDelta = "delta";
auto constexpr kState = "state";
auto constexpr kStats = "stats";
auto constexpr kPath = "path";
auto constexpr kReconnectThread = "FsdbReconnectThread";
auto constexpr kSubscriberThread = "FsdbSubscriberThread";
auto constexpr kStatsPublisherThread = "FsdbStatsPublisherThread";
auto constexpr kStatePublisherThread = "FsdbStatePublisherThread";

std::string toSubscriptionStr(
    const std::string& fsdbHost,
    const std::vector<std::string>& path,
    bool isDelta,
    bool subscribeStats) {
  return folly::to<std::string>(
      fsdbHost,
      ":/",
      (isDelta ? kDelta : kPath),
      ":/",
      (subscribeStats ? kStats : kState),
      ":/",
      folly::join('/', path.begin(), path.end()));
}

std::string toSubscriptionStr(
    const std::string& fsdbHost,
    const std::vector<ExtendedOperPath>& paths,
    bool isDelta,
    bool subscribeStats) {
  return folly::to<std::string>(
      fsdbHost,
      ":/",
      (isDelta ? kDelta : kPath),
      ":/",
      (subscribeStats ? kStats : kState),
      ":/",
      extendedPathsStr(paths));
}
std::vector<ExtendedOperPath> toExtendedOperPath(
    const std::vector<std::vector<std::string>>& paths) {
  std::vector<ExtendedOperPath> extPaths;
  for (const auto& path : paths) {
    ExtendedOperPath extPath;
    for (const auto& pathElm : path) {
      OperPathElem operPathElm;
      operPathElm.raw_ref() = pathElm;
      extPath.path()->push_back(std::move(operPathElm));
    }
    extPaths.push_back(std::move(extPath));
  }
  return extPaths;
}
} // namespace
namespace facebook::fboss::fsdb {

FsdbPubSubManager::FsdbPubSubManager(
    const std::string& clientId,
    folly::EventBase* reconnectEvb,
    folly::EventBase* subscriberEvb,
    folly::EventBase* statsPublisherEvb,
    folly::EventBase* statePublisherEvb)
    : clientId_(clientId),
      // if event base pointer is passed in, no need to spawn local thread
      reconnectEvbThread_(
          reconnectEvb ? nullptr
                       : std::make_unique<folly::ScopedEventBaseThread>(
                             kReconnectThread)),
      subscriberEvbThread_(
          subscriberEvb ? nullptr
                        : std::make_unique<folly::ScopedEventBaseThread>(
                              kSubscriberThread)),
      statsPublisherStreamEvbThread_(
          statsPublisherEvb ? nullptr
                            : std::make_unique<folly::ScopedEventBaseThread>(
                                  kStatsPublisherThread)),
      statePublisherStreamEvbThread_(
          statePublisherEvb ? nullptr
                            : std::make_unique<folly::ScopedEventBaseThread>(
                                  kStatePublisherThread)),
      // if local thread available, use local event base otherwise use passed in
      reconnectEvb_(
          reconnectEvbThread_ ? reconnectEvbThread_->getEventBase()
                              : reconnectEvb),
      subscriberEvb_(
          subscriberEvbThread_ ? subscriberEvbThread_->getEventBase()
                               : subscriberEvb),
      statsPublisherEvb_(
          statsPublisherStreamEvbThread_
              ? statsPublisherStreamEvbThread_->getEventBase()
              : statsPublisherEvb),
      statePublisherEvb_(
          statePublisherStreamEvbThread_
              ? statePublisherStreamEvbThread_->getEventBase()
              : statePublisherEvb) {}

FsdbPubSubManager::~FsdbPubSubManager() {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  stopPublisher(lk, std::move(stateDeltaPublisher_));
  stopPublisher(lk, std::move(statePathPublisher_));
}

void FsdbPubSubManager::stopPublisher(
    const std::lock_guard<std::mutex>& /*lk*/,
    std::unique_ptr<FsdbStreamClient> publisher) {
  if (publisher) {
    publisher->cancel();
  }
}

template <typename PublisherT>
std::unique_ptr<PublisherT> FsdbPubSubManager::createPublisherImpl(
    const std::lock_guard<std::mutex>& /*lk*/,
    const Path& publishPath,
    bool publishStats,
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
    int32_t fsdbPort) const {
  auto publisherExists = publishStats
      ? (statDeltaPublisher_ || statPathPublisher_)
      : (stateDeltaPublisher_ || statePathPublisher_);

  if (publisherExists) {
    throw std::runtime_error(
        "Only one instance of delta or state publisher allowed");
  }
  auto& publisherEvb = publishStats ? statsPublisherEvb_ : statePublisherEvb_;
  auto publisher = std::make_unique<PublisherT>(
      clientId_,
      publishPath,
      publisherEvb,
      reconnectEvb_,
      publishStats,
      publisherStateChangeCb);
  publisher->setServerOptions(FsdbStreamClient::ServerOptions("::1", fsdbPort));
  return publisher;
}

void FsdbPubSubManager::createStateDeltaPublisher(
    const Path& publishPath,
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
    int32_t fsdbPort) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  stateDeltaPublisher_ = createPublisherImpl<FsdbDeltaPublisher>(
      lk,
      publishPath,
      false /*subscribeStat*/,
      publisherStateChangeCb,
      fsdbPort);
}

void FsdbPubSubManager::createStatePathPublisher(
    const Path& publishPath,
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
    int32_t fsdbPort) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  statePathPublisher_ = createPublisherImpl<FsdbStatePublisher>(
      lk,
      publishPath,
      false /*subscribeStat*/,
      publisherStateChangeCb,
      fsdbPort);
}

void FsdbPubSubManager::createStatDeltaPublisher(
    const Path& publishPath,
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
    int32_t fsdbPort) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  statDeltaPublisher_ = createPublisherImpl<FsdbDeltaPublisher>(
      lk,
      publishPath,
      true /*subscribeStat*/,
      publisherStateChangeCb,
      fsdbPort);
}

void FsdbPubSubManager::createStatPathPublisher(
    const Path& publishPath,
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
    int32_t fsdbPort) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  statPathPublisher_ = createPublisherImpl<FsdbStatePublisher>(
      lk,
      publishPath,
      true /*subscribeStat*/,
      publisherStateChangeCb,
      fsdbPort);
}

void FsdbPubSubManager::removeStateDeltaPublisher() {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  stateDeltaPublisher_.reset();
}
void FsdbPubSubManager::removeStatePathPublisher() {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  statePathPublisher_.reset();
}
void FsdbPubSubManager::removeStatDeltaPublisher() {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  statDeltaPublisher_.reset();
}
void FsdbPubSubManager::removeStatPathPublisher() {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  statPathPublisher_.reset();
}

template <typename PublisherT, typename PubUnitT>
void FsdbPubSubManager::publishImpl(PublisherT* publisher, PubUnitT&& pubUnit) {
  if (!publisher) {
    throw std::runtime_error("Publisher must be created before publishing");
  }
  publisher->write(std::forward<PubUnitT>(pubUnit));
}

void FsdbPubSubManager::publishState(OperDelta&& pubUnit) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  publishImpl(stateDeltaPublisher_.get(), std::move(pubUnit));
}

void FsdbPubSubManager::publishState(OperState&& pubUnit) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  publishImpl(statePathPublisher_.get(), std::move(pubUnit));
}

void FsdbPubSubManager::publishStat(OperDelta&& pubUnit) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  publishImpl(statDeltaPublisher_.get(), std::move(pubUnit));
}

void FsdbPubSubManager::publishStat(OperState&& pubUnit) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  publishImpl(statPathPublisher_.get(), std::move(pubUnit));
}

void FsdbPubSubManager::addStateDeltaSubscription(
    const Path& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  addSubscriptionImpl<FsdbDeltaSubscriber>(
      subscribePath,
      stateChangeCb,
      operDeltaCb,
      false /*subscribeStat*/,
      fsdbHost,
      fsdbPort);
}

void FsdbPubSubManager::addStatePathSubscription(
    const Path& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    FsdbStateSubscriber::FsdbOperStateUpdateCb operStateCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  addSubscriptionImpl<FsdbStateSubscriber>(
      subscribePath,
      stateChangeCb,
      operStateCb,
      false /*subscribeStat*/,
      fsdbHost,
      fsdbPort);
}

void FsdbPubSubManager::addStatDeltaSubscription(
    const Path& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  addSubscriptionImpl<FsdbDeltaSubscriber>(
      subscribePath,
      stateChangeCb,
      operDeltaCb,
      true /*subscribeStat*/,
      fsdbHost,
      fsdbPort);
}

void FsdbPubSubManager::addStatPathSubscription(
    const Path& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    FsdbStateSubscriber::FsdbOperStateUpdateCb operStateCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  addSubscriptionImpl<FsdbStateSubscriber>(
      subscribePath,
      stateChangeCb,
      operStateCb,
      true /*subscribeStat*/,
      fsdbHost,
      fsdbPort);
}
/* multi path subscriptions */
void FsdbPubSubManager::addStateDeltaSubscription(
    const MultiPath& subscribePaths,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  addSubscriptionImpl<FsdbExtDeltaSubscriber>(
      toExtendedOperPath(subscribePaths),
      stateChangeCb,
      operDeltaCb,
      false /*subscribeStat*/,
      fsdbHost,
      fsdbPort);
}

void FsdbPubSubManager::addStatePathSubscription(
    const MultiPath& subscribePaths,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    FsdbExtStateSubscriber::FsdbOperStateUpdateCb operStateCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  addSubscriptionImpl<FsdbExtStateSubscriber>(
      toExtendedOperPath(subscribePaths),
      stateChangeCb,
      operStateCb,
      false /*subscribeStat*/,
      fsdbHost,
      fsdbPort);
}

void FsdbPubSubManager::addStatDeltaSubscription(
    const MultiPath& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    FsdbExtDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  addSubscriptionImpl<FsdbExtDeltaSubscriber>(
      toExtendedOperPath(subscribePath),
      stateChangeCb,
      operDeltaCb,
      true /*subscribeStat*/,
      fsdbHost,
      fsdbPort);
}

void FsdbPubSubManager::addStatPathSubscription(
    const MultiPath& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    FsdbExtStateSubscriber::FsdbOperStateUpdateCb operStateCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  addSubscriptionImpl<FsdbExtStateSubscriber>(
      toExtendedOperPath(subscribePath),
      stateChangeCb,
      operStateCb,
      true /*subscribeStat*/,
      fsdbHost,
      fsdbPort);
}

template <typename SubscriberT, typename PathElement>
void FsdbPubSubManager::addSubscriptionImpl(
    const std::vector<PathElement>& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    typename SubscriberT::FsdbSubUnitUpdateCb subUnitAvailableCb,
    bool subscribeStats,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  auto isDelta = std::disjunction_v<
      std::is_same<SubscriberT, FsdbDeltaSubscriber>,
      std::is_same<SubscriberT, FsdbExtDeltaSubscriber>>;
  auto subsStr =
      toSubscriptionStr(fsdbHost, subscribePath, isDelta, subscribeStats);
  path2Subscriber_.withWLock([&](auto& path2Subscriber) {
    auto [itr, inserted] = path2Subscriber.emplace(std::make_pair(
        subsStr,
        std::make_unique<SubscriberT>(
            clientId_,
            subscribePath,
            subscriberEvb_,
            reconnectEvb_,
            subUnitAvailableCb,
            subscribeStats,
            stateChangeCb)));
    if (!inserted) {
      throw std::runtime_error(
          "Subscription at : " + subsStr + " already exists");
    }
    XLOG(DBG2) << " Added subscription for: " << subsStr;
    itr->second->setServerOptions(
        FsdbStreamClient::ServerOptions(fsdbHost, fsdbPort));
  });
}

void FsdbPubSubManager::removeStateDeltaSubscription(
    const Path& subscribePath,
    const std::string& fsdbHost) {
  removeSubscriptionImpl(
      subscribePath, fsdbHost, true /*delta*/, false /*subscribeStats*/);
}
void FsdbPubSubManager::removeStatePathSubscription(
    const Path& subscribePath,
    const std::string& fsdbHost) {
  removeSubscriptionImpl(
      subscribePath, fsdbHost, false /*delta*/, false /*subscribeStats*/);
}
void FsdbPubSubManager::removeStatDeltaSubscription(
    const Path& subscribePath,
    const std::string& fsdbHost) {
  removeSubscriptionImpl(
      subscribePath, fsdbHost, true /*delta*/, true /*subscribeStats*/);
}
void FsdbPubSubManager::removeStatPathSubscription(
    const Path& subscribePath,
    const std::string& fsdbHost) {
  removeSubscriptionImpl(
      subscribePath, fsdbHost, false /*delta*/, true /*subscribeStats*/);
}

void FsdbPubSubManager::removeStateDeltaSubscription(
    const MultiPath& subscribePath,
    const std::string& fsdbHost) {
  removeSubscriptionImpl(
      toExtendedOperPath(subscribePath),
      fsdbHost,
      true /*delta*/,
      false /*subscribeStats*/);
}
void FsdbPubSubManager::removeStatePathSubscription(
    const MultiPath& subscribePath,
    const std::string& fsdbHost) {
  removeSubscriptionImpl(
      toExtendedOperPath(subscribePath),
      fsdbHost,
      false /*delta*/,
      false /*subscribeStats*/);
}
void FsdbPubSubManager::removeStatDeltaSubscription(
    const MultiPath& subscribePath,
    const std::string& fsdbHost) {
  removeSubscriptionImpl(
      toExtendedOperPath(subscribePath),
      fsdbHost,
      true /*delta*/,
      true /*subscribeStats*/);
}
void FsdbPubSubManager::removeStatPathSubscription(
    const MultiPath& subscribePath,
    const std::string& fsdbHost) {
  removeSubscriptionImpl(
      toExtendedOperPath(subscribePath),
      fsdbHost,
      false /*delta*/,
      true /*subscribeStats*/);
}
template <typename PathElement>
void FsdbPubSubManager::removeSubscriptionImpl(
    const std::vector<PathElement>& subscribePath,
    const std::string& fsdbHost,
    bool isDelta,
    bool subscribeStats) {
  auto subsStr =
      toSubscriptionStr(fsdbHost, subscribePath, isDelta, subscribeStats);
  if (path2Subscriber_.wlock()->erase(subsStr)) {
    XLOG(DBG2) << "Erased subscription for : " << subsStr;
  }
}

} // namespace facebook::fboss::fsdb
