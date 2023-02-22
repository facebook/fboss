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

class Sandia16QPimPlatformMapping : public MultiPimPlatformMapping {
 public:
  Sandia16QPimPlatformMapping();
  explicit Sandia16QPimPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Sandia16QPimPlatformMapping(Sandia16QPimPlatformMapping const&) = delete;
  Sandia16QPimPlatformMapping& operator=(Sandia16QPimPlatformMapping const&) =
      delete;
};
} // namespace fboss
} // namespace facebook
