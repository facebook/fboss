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

class Leh800bclsPlatformMapping : public PlatformMapping {
 public:
  Leh800bclsPlatformMapping();
  explicit Leh800bclsPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Leh800bclsPlatformMapping(Leh800bclsPlatformMapping const&) = delete;
  Leh800bclsPlatformMapping& operator=(Leh800bclsPlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
