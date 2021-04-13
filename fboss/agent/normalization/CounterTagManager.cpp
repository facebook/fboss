/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/normalization/CounterTagManager.h"

#include <folly/MapUtil.h>

namespace facebook::fboss::normalization {

void CounterTagManager::reloadCounterTags(const cfg::SwitchConfig& curConfig) {
  PortCounterTagsMap newPortCounterTags;
  for (auto& port : *curConfig.ports_ref()) {
    if (port.name_ref() && port.counterTags_ref()) {
      auto tags =
          std::make_shared<std::vector<std::string>>(*port.counterTags_ref());
      newPortCounterTags.emplace(*port.name_ref(), std::move(tags));
    }
  }

  // replace old cache
  portCounterTags_ = std::move(newPortCounterTags);
}

std::shared_ptr<std::vector<std::string>> CounterTagManager::getCounterTags(
    const std::string& portName) const {
  auto locked = portCounterTags_.rlock();
  if (auto ptr = folly::get_ptr(*locked, portName)) {
    return *ptr;
  }
  return nullptr;
}

} // namespace facebook::fboss::normalization
