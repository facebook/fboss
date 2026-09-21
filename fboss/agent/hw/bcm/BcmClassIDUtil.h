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

#include "fboss/agent/hw/bcm/types.h"

namespace facebook::fboss {

class BcmClassIDUtil {
 public:
  static bool isValidLookupClass(cfg::AclLookupClass classID);
  static bool isValidQueuePerHostClass(cfg::AclLookupClass classID);
};

} // namespace facebook::fboss
