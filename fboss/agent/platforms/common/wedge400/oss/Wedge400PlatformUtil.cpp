/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/common/wedge400/Wedge400PlatformUtil.h"

namespace facebook::fboss::utility {
bool isWedge400PlatformRackTypeInference() {
  return false;
}

bool isWedge400PlatformRackTypeAcadia() {
  return false;
}
} // namespace facebook::fboss::utility
