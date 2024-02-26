/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include <memory>
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/test/utils/AclTestUtils.h"

#include "fboss/agent/state/SwitchState.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss::utility {

std::vector<cfg::CounterType> getAclCounterTypes(const HwAsic* asic) {
  // At times, it is non-trivial for SAI implementations to support enabling
  // bytes counters only or packet counters only. In such cases, SAI
  // implementations enable bytes as well as packet counters even if only
  // one of the two is enabled. FBOSS use case does not require enabling
  // only one, but always enables both packets and bytes counters. Thus,
  // enable both in the test. Reference: CS00012271364
  if (asic->isSupported(
          HwAsic::Feature::SEPARATE_BYTE_AND_PACKET_ACL_COUNTER)) {
    return {cfg::CounterType::PACKETS};
  } else {
    return {cfg::CounterType::BYTES, cfg::CounterType::PACKETS};
  }
}

} // namespace facebook::fboss::utility
