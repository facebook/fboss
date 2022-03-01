// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/client/FsdbPubSubManager.h"

#include <string>
#include "fboss/fsdb/client/FsdbDeltaPublisher.h"

namespace facebook::fboss::fsdb {

void FsdbPubSubManager::createDeltaPublisher(
    const std::vector<std::string>& publishPath,
    FsdbStreamClient::FsdbStreamStateChangeCb publisherStateChangeCb) {
  stopDeltaPublisher();
  deltaPublisher_ = std::make_unique<FsdbDeltaPublisher>(
      clientId_,
      publishPath,
      publisherStreamEvbThread_.getEventBase(),
      reconnectThread_.getEventBase(),
      publisherStateChangeCb);
}

void FsdbPubSubManager::stopDeltaPublisher() {
  if (deltaPublisher_) {
    deltaPublisher_->cancel();
    deltaPublisher_.reset();
  }
}

void FsdbPubSubManager::publish(const OperDelta& pubUnit) {
  CHECK(deltaPublisher_);
  deltaPublisher_->write(pubUnit);
}
} // namespace facebook::fboss::fsdb
