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
#include "fboss/agent/platforms/wedge/WedgePort.h"
#include "fboss/agent/ThriftHandler.h"

#include <folly/Memory.h>

namespace facebook { namespace fboss {

std::unique_ptr<ThriftHandler> WedgePlatform::createHandler(SwSwitch* sw) {
  return std::make_unique<ThriftHandler>(sw);
}

std::map<std::string, std::string> WedgePlatform::loadConfig() {
  std::map<std::string, std::string> config;
  return config;
}

void WedgePlatform::onHwInitialized(SwSwitch* /*sw*/) {
  // TODO: Initialize the LEDs.  The LED handling code isn't open source yet,
  // but should be soon once we get approval for the required OpenNSL APIs.
}

}} // facebook::fboss
