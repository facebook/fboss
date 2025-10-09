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

class Meru800bfaPlatformMapping : public PlatformMapping {
 public:
  Meru800bfaPlatformMapping();
  explicit Meru800bfaPlatformMapping(const std::string& platformMappingStr);
  explicit Meru800bfaPlatformMapping(
      bool multiNpuPlatformMapping,
      std::optional<int> version = std::nullopt);

 private:
  // Forbidden copy constructor and assignment operator
  Meru800bfaPlatformMapping(Meru800bfaPlatformMapping const&) = delete;
  Meru800bfaPlatformMapping& operator=(Meru800bfaPlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
