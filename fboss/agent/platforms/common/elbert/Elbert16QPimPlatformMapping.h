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

namespace facebook::fboss {

class Elbert16QPimPlatformMapping : public MultiPimPlatformMapping {
 public:
  Elbert16QPimPlatformMapping();
  explicit Elbert16QPimPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Elbert16QPimPlatformMapping(Elbert16QPimPlatformMapping const&) = delete;
  Elbert16QPimPlatformMapping& operator=(Elbert16QPimPlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
