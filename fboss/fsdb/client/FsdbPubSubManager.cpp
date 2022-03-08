// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <string>
#include "fboss/fsdb/client/FsdbDeltaPublisher.h"

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

FsdbPubSubManager::~FsdbPubSubManager() {}

void FsdbPubSubManager::createDeltaPublisher(
    const std::vector<std::string>& publishPath,
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb,
    int32_t fsdbPort) {
  deltaPublisher_.withWLock([&](auto& deltaPublisher) {
    stopDeltaPublisher(deltaPublisher);
    deltaPublisher = std::make_unique<FsdbDeltaPublisher>(
        clientId_,
        publishPath,
        publisherStreamEvbThread_.getEventBase(),
        reconnectThread_.getEventBase(),
        publisherStateChangeCb);
    deltaPublisher->setServerToConnect("::1", fsdbPort);
  });
}

void FsdbPubSubManager::stopDeltaPublisher(
    std::unique_ptr<FsdbDeltaPublisher>& deltaPublisher) {
  if (deltaPublisher) {
    deltaPublisher->cancel();
    deltaPublisher.reset();
  }
}

void FsdbPubSubManager::publish(const OperDelta& pubUnit) {
  deltaPublisher_.withWLock([&pubUnit](auto& deltaPublisher) {
    CHECK(deltaPublisher);
    deltaPublisher->write(pubUnit);
  });
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
