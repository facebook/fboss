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

namespace facebook {
namespace fboss {

class Wedge100PlatformMapping : public PlatformMapping {
 public:
  Wedge100PlatformMapping();
  explicit Wedge100PlatformMapping(const std::string& platformMappingStr);

  void customizePlatformPortConfigOverrideFactor(
      std::optional<cfg::PlatformPortConfigOverrideFactor>& factor)
      const override;

 private:
  // Forbidden copy constructor and assignment operator
  Wedge100PlatformMapping(Wedge100PlatformMapping const&) = delete;
  Wedge100PlatformMapping& operator=(Wedge100PlatformMapping const&) = delete;
};
} // namespace fboss
} // namespace facebook
