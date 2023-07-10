/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/WedgePlatform.h"

#include "fboss/agent/ThriftHandler.h"

namespace facebook::fboss {

std::unique_ptr<ThriftHandler> WedgePlatform::createHandler(SwSwitch* sw) {
  return std::make_unique<ThriftHandler>(sw);
}

std::shared_ptr<apache::thrift::AsyncProcessorFactory>
WedgePlatform::createHandler() {
  return nullptr;
}

void WedgePlatform::initLEDs() {
  // TODO: Initialize the LEDs.  The LED handling code isn't open source yet,
  // but should be soon once we get approval for the required Bcm APIs.
}

} // namespace facebook::fboss
