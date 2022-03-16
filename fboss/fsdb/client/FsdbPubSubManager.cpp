// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <string>
#include "fboss/fsdb/client/FsdbDeltaPublisher.h"
#include "fboss/fsdb/client/FsdbStatePublisher.h"

namespace {
auto constexpr kDelta = "delta";
auto constexpr kState = "state";
auto constexpr kStats = "stats";
auto constexpr kPath = "path";

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
} // namespace
namespace facebook::fboss::fsdb {

FsdbPubSubManager::FsdbPubSubManager(const std::string& clientId)
    : clientId_(clientId) {}

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
    const std::vector<std::string>& publishPath,
    bool publishStats,
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
    int32_t fsdbPort) const {
  if (stateDeltaPublisher_ || statePathPublisher_) {
    throw std::runtime_error(
        "Only one instance of delta or state publisher allowed");
  }
  auto& evbThread = publishStats ? statsPublisherStreamEvbThread_
                                 : statePublisherStreamEvbThread_;
  auto publisher = std::make_unique<PublisherT>(
      clientId_,
      publishPath,
      evbThread.getEventBase(),
      reconnectThread_.getEventBase(),
      publishStats,
      publisherStateChangeCb);
  publisher->setServerToConnect("::1", fsdbPort);
  return publisher;
}

void FsdbPubSubManager::createStateDeltaPublisher(
    const std::vector<std::string>& publishPath,
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
    const std::vector<std::string>& publishPath,
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
    const std::vector<std::string>& publishPath,
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
    const std::vector<std::string>& publishPath,
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

template <typename PublisherT, typename PubUnitT>
void FsdbPubSubManager::publishImpl(
    PublisherT* publisher,
    const PubUnitT& pubUnit) {
  if (!publisher) {
    throw std::runtime_error("Publisher must be created before publishing");
  }
  std::lock_guard<std::mutex> lk(publisherMutex_);
  publisher->write(pubUnit);
}

void FsdbPubSubManager::publishState(const OperDelta& pubUnit) {
  publishImpl(stateDeltaPublisher_.get(), pubUnit);
}

void FsdbPubSubManager::publishState(const OperState& pubUnit) {
  publishImpl(statePathPublisher_.get(), pubUnit);
}

void FsdbPubSubManager::publishStat(const OperDelta& pubUnit) {
  publishImpl(statDeltaPublisher_.get(), pubUnit);
}

void FsdbPubSubManager::publishStat(const OperState& pubUnit) {
  publishImpl(statPathPublisher_.get(), pubUnit);
}

void FsdbPubSubManager::addStateDeltaSubscription(
    const std::vector<std::string>& subscribePath,
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
    const std::vector<std::string>& subscribePath,
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
    const std::vector<std::string>& subscribePath,
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
    const std::vector<std::string>& subscribePath,
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

template <typename SubscriberT>
void FsdbPubSubManager::addSubscriptionImpl(
    const std::vector<std::string>& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    typename SubscriberT::FsdbSubUnitUpdateCb subUnitAvailableCb,
    bool subscribeStats,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  auto isDelta = std::is_same_v<SubscriberT, FsdbDeltaSubscriber>;
  auto subsStr =
      toSubscriptionStr(fsdbHost, subscribePath, isDelta, subscribeStats);
  path2Subscriber_.withWLock([&](auto& path2Subscriber) {
    auto [itr, inserted] = path2Subscriber.emplace(std::make_pair(
        subsStr,
        std::make_unique<SubscriberT>(
            clientId_,
            subscribePath,
            subscribersStreamEvbThread_.getEventBase(),
            reconnectThread_.getEventBase(),
            subUnitAvailableCb,
            subscribeStats,
            stateChangeCb)));
    if (!inserted) {
      throw std::runtime_error(
          "Subscription at : " + subsStr + " already exists");
    }
    XLOG(DBG2) << " Added subscription for: " << subsStr;
    itr->second->setServerToConnect(fsdbHost, fsdbPort);
  });
}

void FsdbPubSubManager::removeSubscriptionImpl(
    const std::vector<std::string>& subscribePath,
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
