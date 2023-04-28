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

namespace facebook {
namespace fboss {

class Morgan800ccPlatformMapping : public PlatformMapping {
 public:
  Morgan800ccPlatformMapping();
  explicit Morgan800ccPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Morgan800ccPlatformMapping(Morgan800ccPlatformMapping const&) = delete;
  Morgan800ccPlatformMapping& operator=(Morgan800ccPlatformMapping const&) = delete;
};
} // namespace fboss
} // namespace facebook