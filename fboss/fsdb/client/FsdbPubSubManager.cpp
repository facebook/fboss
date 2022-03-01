// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <string>
#include "fboss/fsdb/client/FsdbDeltaPublisher.h"

namespace facebook::fboss::fsdb {

FsdbPubSubManager::FsdbPubSubManager(const std::string& clientId)
    : clientId_(clientId) {}

FsdbPubSubManager::~FsdbPubSubManager() {
  deltaPublisher_.withWLock(
      [this](auto& deltaPublisher) { stopDeltaPublisher(deltaPublisher); });
}

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
} // namespace facebook::fboss::fsdb
