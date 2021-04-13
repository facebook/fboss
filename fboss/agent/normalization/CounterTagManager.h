/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <folly/Synchronized.h>
#include <folly/container/F14Map.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss::normalization {
/*
 * class to handle and manage counter tags to make it easier for rest of
 * normalization layer to access counter tags which currently reside in agent
 * config
 */
class CounterTagManager {
 public:
  // Will be called whenever there is an agent config update that signals we
  // need to rebuild our in-memory cache
  void reloadCounterTags(const cfg::SwitchConfig& curConfig);

  // get counter tags for a given port
  std::shared_ptr<std::vector<std::string>> getCounterTags(
      const std::string& portName) const;

 private:
  // {port name -> list of counter tags}
  using PortCounterTagsMap =
      folly::F14FastMap<std::string, std::shared_ptr<std::vector<std::string>>>;

  // in-memory cache maintaining map of port counter tags
  folly::Synchronized<PortCounterTagsMap> portCounterTags_;
};

} // namespace facebook::fboss::normalization
