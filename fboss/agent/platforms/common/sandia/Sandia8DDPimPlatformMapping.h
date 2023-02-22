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

class Sandia8DDPimPlatformMapping : public MultiPimPlatformMapping {
 public:
  Sandia8DDPimPlatformMapping();
  explicit Sandia8DDPimPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Sandia8DDPimPlatformMapping(Sandia8DDPimPlatformMapping const&) = delete;
  Sandia8DDPimPlatformMapping& operator=(Sandia8DDPimPlatformMapping const&) =
      delete;
};
} // namespace fboss
} // namespace facebook
