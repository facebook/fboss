// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <string>
#include "fboss/fsdb/client/FsdbDeltaPublisher.h"

namespace {

std::string toPathStr(
    const std::string& fsdbHost,
    const std::vector<std::string>& path) {
  return fsdbHost + ":/" + folly::join('/', path.begin(), path.end());
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
  auto pathStr = toPathStr(fsdbHost, subscribePath);
  path2Subscriber_.withWLock([&](auto& path2Subscriber) {
    auto [itr, inserted] = path2Subscriber.emplace(std::make_pair(
        pathStr,
        std::make_unique<FsdbDeltaSubscriber>(
            clientId_,
            subscribePath,
            subscribersStreamEvbThread_.getEventBase(),
            reconnectThread_.getEventBase(),
            operDeltaCb,
            stateChangeCb)));
    if (!inserted) {
      throw std::runtime_error(
          "Subscription at : " + pathStr + " already exists");
    }
    XLOG(DBG2) << " Added subscription for: " << pathStr;
    itr->second->setServerToConnect(fsdbHost, fsdbPort);
  });
}

void FsdbPubSubManager::removeSubscription(
    const std::vector<std::string>& subscribePath,
    const std::string& fsdbHost) {
  auto pathStr = toPathStr(fsdbHost, subscribePath);
  if (path2Subscriber_.wlock()->erase(pathStr)) {
    XLOG(DBG2) << "Erased subscription for : " << pathStr;
  }
}
} // namespace facebook::fboss::fsdb
