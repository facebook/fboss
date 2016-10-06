/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/ThriftHandler.h"

namespace facebook { namespace fboss {

std::unique_ptr<ThriftHandler> SaiPlatform::createHandler(SwSwitch *sw) {
  return folly::make_unique<ThriftHandler>(sw);
}

std::map<std::string, std::string> SaiPlatform::loadConfig() {
  std::map<std::string, std::string> config;
  return std::move(config);
}

void SaiPlatform::onHwInitialized(SwSwitch *sw) {
}

}} // facebook::fboss
