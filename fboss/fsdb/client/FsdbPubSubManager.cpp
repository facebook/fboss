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

std::string toSubscriptionStr(
    const std::string& fsdbHost,
    const std::vector<std::string>& path,
    bool isDelta) {
  return folly::to<std::string>(
      fsdbHost,
      ":/",
      (isDelta ? kDelta : kState),
      ":/",
      folly::join('/', path.begin(), path.end()));
}
} // namespace
namespace facebook::fboss::fsdb {

FsdbPubSubManager::FsdbPubSubManager(const std::string& clientId)
    : clientId_(clientId) {}

FsdbPubSubManager::~FsdbPubSubManager() {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  stopPublisher(lk, std::move(deltaPublisher_));
  stopPublisher(lk, std::move(statePublisher_));
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
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
    int32_t fsdbPort) const {
  if (deltaPublisher_ || statePublisher_) {
    throw std::runtime_error(
        "Only one instance of delta or state publisher allowed");
  }
  auto publisher = std::make_unique<PublisherT>(
      clientId_,
      publishPath,
      publisherStreamEvbThread_.getEventBase(),
      reconnectThread_.getEventBase(),
      publisherStateChangeCb);
  publisher->setServerToConnect("::1", fsdbPort);
  return publisher;
}

void FsdbPubSubManager::createDeltaPublisher(
    const std::vector<std::string>& publishPath,
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
    int32_t fsdbPort) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  deltaPublisher_ = createPublisherImpl<FsdbDeltaPublisher>(
      lk, publishPath, publisherStateChangeCb, fsdbPort);
}

void FsdbPubSubManager::createStatePublisher(
    const std::vector<std::string>& publishPath,
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
    int32_t fsdbPort) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  statePublisher_ = createPublisherImpl<FsdbStatePublisher>(
      lk, publishPath, publisherStateChangeCb, fsdbPort);
}

template <typename PublisherT, typename PubUnitT>
void FsdbPubSubManager::publishImpl(
    const std::lock_guard<std::mutex>& /*lk*/,
    PublisherT* publisher,
    const PubUnitT& pubUnit) {
  if (!publisher) {
    throw std::runtime_error("Publisher must be created before publishing");
  }
  publisher->write(pubUnit);
}

void FsdbPubSubManager::publish(const OperDelta& pubUnit) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  publishImpl(lk, deltaPublisher_.get(), pubUnit);
}

void FsdbPubSubManager::publish(const OperState& pubUnit) {
  std::lock_guard<std::mutex> lk(publisherMutex_);
  publishImpl(lk, statePublisher_.get(), pubUnit);
}

void FsdbPubSubManager::addSubscription(
    const std::vector<std::string>& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    FsdbDeltaSubscriber::FsdbOperDeltaUpdateCb operDeltaCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  addSubscriptionImpl<FsdbDeltaSubscriber>(
      subscribePath, stateChangeCb, operDeltaCb, fsdbHost, fsdbPort);
}

void FsdbPubSubManager::addSubscription(
    const std::vector<std::string>& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    FsdbStateSubscriber::FsdbOperStateUpdateCb operStateCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  addSubscriptionImpl<FsdbStateSubscriber>(
      subscribePath, stateChangeCb, operStateCb, fsdbHost, fsdbPort);
}

template <typename SubscriberT>
void FsdbPubSubManager::addSubscriptionImpl(
    const std::vector<std::string>& subscribePath,
    FsdbStreamClient::FsdbStreamStateChangeCb stateChangeCb,
    typename SubscriberT::FsdbSubUnitUpdateCb subUnitAvailableCb,
    const std::string& fsdbHost,
    int32_t fsdbPort) {
  auto isDelta = std::is_same_v<SubscriberT, FsdbDeltaSubscriber>;
  auto subsStr = toSubscriptionStr(fsdbHost, subscribePath, isDelta);
  path2Subscriber_.withWLock([&](auto& path2Subscriber) {
    auto [itr, inserted] = path2Subscriber.emplace(std::make_pair(
        subsStr,
        std::make_unique<SubscriberT>(
            clientId_,
            subscribePath,
            subscribersStreamEvbThread_.getEventBase(),
            reconnectThread_.getEventBase(),
            subUnitAvailableCb,
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
    bool isDelta) {
  auto subsStr = toSubscriptionStr(fsdbHost, subscribePath, isDelta);
  if (path2Subscriber_.wlock()->erase(subsStr)) {
    XLOG(DBG2) << "Erased subscription for : " << subsStr;
  }
}
} // namespace facebook::fboss::fsdb
