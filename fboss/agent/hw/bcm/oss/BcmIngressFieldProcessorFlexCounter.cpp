/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmIngressFieldProcessorFlexCounter.h"

#include "fboss/agent/hw/bcm/BcmError.h"

namespace facebook::fboss {
BcmIngressFieldProcessorFlexCounter::BcmIngressFieldProcessorFlexCounter(
    int unit,
    int /* groupID */)
    : BcmFlexCounter(unit) {
  throw FbossError("OSS doesn't support creating IFP flex counter");
}

void BcmIngressFieldProcessorFlexCounter::attach(BcmAclEntryHandle /* acl */) {
  throw FbossError("OSS doesn't support attach IFP flex counter to acl");
}

void BcmIngressFieldProcessorFlexCounter::detach(
    int /* unit */,
    BcmAclEntryHandle /* acl */,
    BcmAclStatHandle /* aclStatHandle */) {
  throw FbossError("OSS doesn't support detach IFP flex counter from acl");
}

std::set<cfg::CounterType>
BcmIngressFieldProcessorFlexCounter::getCounterTypeList(
    int /* unit */,
    uint32_t /* counterID */) {
  throw FbossError("OSS doesn't support get counter types of IFP flex counter");
}
} // namespace facebook::fboss
