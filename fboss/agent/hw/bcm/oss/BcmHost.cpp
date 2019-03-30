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

bool BcmHost::matchLookupClass(
    const opennsl_l3_host_t& /*newHost*/,
    const opennsl_l3_host_t& /*existingHost*/) {
  // opennsl doesn't support lookup class, so no need to compare and always
  // returns true
  return true;
}
// Since opennsl doesn't support lookup class, no ops
void BcmHost::setLookupClassToL3Host(opennsl_l3_host_t* /*host*/) const {}
int BcmHost::getLookupClassFromL3Host(const opennsl_l3_host_t& /*host*/) {
  return 0;
}
}} // facebook::fboss
