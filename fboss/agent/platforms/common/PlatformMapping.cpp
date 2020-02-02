/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/PlatformMapping.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook {
namespace fboss {
PlatformMapping::PlatformMapping(const std::string& jsonPlatformMappingStr) {
  auto mapping =
      apache::thrift::SimpleJSONSerializer::deserialize<cfg::PlatformMapping>(
          jsonPlatformMappingStr);
  platformPorts_ = std::move(mapping.ports);
  supportedProfiles_ = std::move(mapping.supportedProfiles);
  for (auto chip : mapping.chips) {
    chips_[chip.name] = chip;
  }
}

void PlatformMapping::merge(PlatformMapping* mapping) {
  for (auto port : mapping->platformPorts_) {
    platformPorts_.emplace(port.first, std::move(port.second));
  }
  mapping->platformPorts_.clear();

  for (auto profile : mapping->supportedProfiles_) {
    supportedProfiles_.emplace(profile.first, std::move(profile.second));
  }
  mapping->supportedProfiles_.clear();

  for (auto chip : mapping->chips_) {
    chips_.emplace(chip.first, std::move(chip.second));
  }
  mapping->chips_.clear();
}
} // namespace fboss
} // namespace facebook
