/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/BcmTrunkUtils.h"

#include "fboss/agent/hw/bcm/BcmError.h"

#include <vector>

namespace facebook::fboss::utility {

int getBcmTrunkMemberCount(int unit, bcm_trunk_t tid, int swSubportCount) {
  bcm_trunk_info_t trunk_info;
  // use vector instead of a c-array to stop the linter from nagging
  std::vector<bcm_trunk_member_t> members(swSubportCount);
  int hwMemberCount;
  auto rv = bcm_trunk_get(
      unit, tid, &trunk_info, swSubportCount, members.data(), &hwMemberCount);
  bcmCheckError(rv, "Unable to get info for trunk : ", tid);
  return hwMemberCount;
}

} // namespace facebook::fboss::utility
