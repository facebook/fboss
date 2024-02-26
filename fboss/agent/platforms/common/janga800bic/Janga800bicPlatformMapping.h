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

class Janga800bicPlatformMapping : public PlatformMapping {
 public:
  Janga800bicPlatformMapping();
  explicit Janga800bicPlatformMapping(const std::string& platformMappingStr);
  explicit Janga800bicPlatformMapping(bool multiNpuPlatformMapping);

 private:
  // Forbidden copy constructor and assignment operator
  Janga800bicPlatformMapping(Janga800bicPlatformMapping const&) = delete;
  Janga800bicPlatformMapping& operator=(Janga800bicPlatformMapping const&) =
      delete;
};
} // namespace facebook::fboss
