// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/util/qsfp/QsfpUtilContainer.h"
#include <folly/Singleton.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

folly::Singleton<QsfpUtilContainer> _qsfpUtilContainer;

std::shared_ptr<QsfpUtilContainer> QsfpUtilContainer::getInstance() {
  return _qsfpUtilContainer.try_get();
}

QsfpUtilContainer::QsfpUtilContainer() {
  detectTransceiverBus();
}

/*
 * This function gets the transceiver bus.
 */
void QsfpUtilContainer::detectTransceiverBus() {
  auto busAndError = getTransceiverAPI();

  if (busAndError.second) {
    throw FbossError(
        fmt::format("Unknown platform error: {:d}", busAndError.second));
  }

  bus_ = std::move(busAndError.first);
}

} // namespace facebook::fboss
