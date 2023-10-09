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

#include "fboss/lib/usb/WedgeI2CBus.h"

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace facebook {
namespace fboss {
class Wedge40Manager : public WedgeManager {
 public:
  explicit Wedge40Manager(const std::string& platformMappingStr);
  ~Wedge40Manager() override {}

 private:
  // Forbidden copy constructor and assignment operator
  Wedge40Manager(Wedge40Manager const&) = delete;
  Wedge40Manager& operator=(Wedge40Manager const&) = delete;
};
} // namespace fboss
} // namespace facebook
