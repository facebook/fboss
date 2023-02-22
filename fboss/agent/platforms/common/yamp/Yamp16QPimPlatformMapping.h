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

namespace facebook {
namespace fboss {

class Yamp16QPimPlatformMapping : public MultiPimPlatformMapping {
 public:
  Yamp16QPimPlatformMapping();
  explicit Yamp16QPimPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Yamp16QPimPlatformMapping(Yamp16QPimPlatformMapping const&) = delete;
  Yamp16QPimPlatformMapping& operator=(Yamp16QPimPlatformMapping const&) =
      delete;
};
} // namespace fboss
} // namespace facebook
