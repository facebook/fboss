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

#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/SwitchState.h"

#include <gtest/gtest.h>

DECLARE_int32(acl_gid);

extern "C" {
#include <bcm/field.h>
}

namespace facebook::fboss::utility {
int getAclTableNumAclEntries(const HwSwitch* hwSwitch) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  bcm_field_group_t gid = FLAGS_acl_gid;
  int size;

  auto rv =
      bcm_field_entry_multi_get(bcmSwitch->getUnit(), gid, 0, nullptr, &size);
  bcmCheckError(
      rv,
      "failed to get field group entry count, gid=",
      folly::to<std::string>(gid));
  return size;
}

void checkSwHwAclMatch(
    const HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  auto swAcl = state->getAcl(aclName);
  ASSERT_NE(nullptr, swAcl);
  auto hwAcl = bcmSwitch->getAclTable()->getAclIf(swAcl->getPriority());
  ASSERT_NE(nullptr, hwAcl);
  ASSERT_TRUE(BcmAclEntry::isStateSame(
      bcmSwitch, FLAGS_acl_gid, hwAcl->getHandle(), swAcl));
}

} // namespace facebook::fboss::utility
