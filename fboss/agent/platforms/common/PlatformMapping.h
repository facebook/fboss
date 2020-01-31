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

#include "fboss/agent/gen-cpp2/platform_config_types.h"

namespace facebook {
namespace fboss {

class PlatformMapping {
 public:
  PlatformMapping() {}
  explicit PlatformMapping(const std::string& jsonPlatformMappingStr);

  const std::map<int32_t, cfg::PlatformPortEntry>& getPlatformPorts() const {
    return platformPorts_;
  }

  const std::map<cfg::PortProfileID, phy::PortProfileConfig>&
  getSupportedProfiles() const {
    return supportedProfiles_;
  }

  const std::map<std::string, phy::DataPlanePhyChip>& getChips() const {
    return chips_;
  }

 protected:
  std::map<int32_t, cfg::PlatformPortEntry> platformPorts_;
  std::map<cfg::PortProfileID, phy::PortProfileConfig> supportedProfiles_;
  std::map<std::string, phy::DataPlanePhyChip> chips_;

 private:
  // Forbidden copy constructor and assignment operator
  PlatformMapping(PlatformMapping const&) = delete;
  PlatformMapping& operator=(PlatformMapping const&) = delete;
};
} // namespace fboss
} // namespace facebook
