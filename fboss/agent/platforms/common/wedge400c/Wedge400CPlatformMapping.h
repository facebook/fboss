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

class Wedge400CPlatformMapping : public PlatformMapping {
 public:
  Wedge400CPlatformMapping();
  explicit Wedge400CPlatformMapping(const std::string& platformMappingStr);

 private:
  // Forbidden copy constructor and assignment operator
  Wedge400CPlatformMapping(Wedge400CPlatformMapping const&) = delete;
  Wedge400CPlatformMapping& operator=(Wedge400CPlatformMapping const&) = delete;
};
} // namespace fboss
} // namespace facebook
