/*
 *  Copyright (c) 2023-present, Facebook, Inc.
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

class YangraPlatformMapping : public PlatformMapping {
 public:
  YangraPlatformMapping();
  explicit YangraPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  YangraPlatformMapping(YangraPlatformMapping const&) = delete;
  YangraPlatformMapping& operator=(YangraPlatformMapping const&) = delete;
};
} // namespace facebook::fboss
