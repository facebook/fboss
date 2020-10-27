/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"

#include "fboss/agent/hw/bcm/BcmError.h"

namespace facebook::fboss {
BcmTrafficCounterStats BcmStatUpdater::getAclTrafficFlexCounterStats(
    BcmAclStatHandle /* handle */,
    const std::vector<cfg::CounterType>& /* counters */) {
  throw FbossError("OSS doesn't support get stat of IFP flex counter");
}
} // namespace facebook::fboss
