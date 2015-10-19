/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmHost.h"

namespace facebook { namespace fboss {

// This should be temporary for the oss build until the needed symbols are added
// to opennsl. Always returning true should be the same as having no expiration.
bool BcmHost::getAndClearHitBit() const {
  return true;
}
}} // facebook::fboss
