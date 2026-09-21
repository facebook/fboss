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

#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook::fboss {

class SaintpaulPlatformMapping : public PlatformMapping {
 public:
  SaintpaulPlatformMapping();
  explicit SaintpaulPlatformMapping(const std::string& platformMappingStr);
  ~SaintpaulPlatformMapping() override = default;
  SaintpaulPlatformMapping(SaintpaulPlatformMapping&&) = delete;
  SaintpaulPlatformMapping& operator=(SaintpaulPlatformMapping&&) = delete;
  // For CPU system port number as key, get the core for CPU port and
  // the port ID within the core.
  std::map<uint32_t, std::pair<uint32_t, uint32_t>> getCpuPortsCoreAndPortIdx()
      const override;

 private:
  // Forbidden copy constructor and assignment operator
  SaintpaulPlatformMapping(SaintpaulPlatformMapping const&) = delete;
  SaintpaulPlatformMapping& operator=(SaintpaulPlatformMapping const&) = delete;
};
} // namespace facebook::fboss
