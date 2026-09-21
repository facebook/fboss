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

#include "fboss/agent/platforms/common/MultiPimPlatformMapping.h"
#include "fboss/lib/platforms/PlatformMode.h"

namespace facebook::fboss {

class Minipack16QPimPlatformMapping : public MultiPimPlatformMapping {
 public:
  explicit Minipack16QPimPlatformMapping(ExternalPhyVersion xphyVersion);
  explicit Minipack16QPimPlatformMapping(const std::string& platformMappingStr);
  ~Minipack16QPimPlatformMapping() = default;

 private:
  // Forbidden copy constructor and assignment operator
  Minipack16QPimPlatformMapping(Minipack16QPimPlatformMapping const&) = delete;
  Minipack16QPimPlatformMapping& operator=(
      Minipack16QPimPlatformMapping const&) = delete;
  Minipack16QPimPlatformMapping(Minipack16QPimPlatformMapping&&) = delete;
  Minipack16QPimPlatformMapping& operator=(Minipack16QPimPlatformMapping&&) =
      delete;
};
} // namespace facebook::fboss
