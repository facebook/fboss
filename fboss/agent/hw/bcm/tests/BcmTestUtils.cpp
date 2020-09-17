/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"

#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/SwitchState.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

void checkSwHwAclMatch(
    BcmSwitch* hw,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName) {
  auto swAcl = state->getAcl(aclName);
  ASSERT_NE(nullptr, swAcl);
  auto hwAcl = hw->getAclTable()->getAclIf(swAcl->getPriority());
  ASSERT_NE(nullptr, hwAcl);
  ASSERT_TRUE(BcmAclEntry::isStateSame(
      hw,
      hw->getPlatform()->getAsic()->getDefaultACLGroupID(),
      hwAcl->getHandle(),
      swAcl));
}

void assertSwitchControl(bcm_switch_control_t type, int expectedValue) {
  int value = 0;

  int rv = bcm_switch_control_get(0, type, &value);
  facebook::fboss::bcmCheckError(rv, "failed to retrieve value for ", type);

  ASSERT_EQ(value, expectedValue) << "type=" << type;
}

} // namespace facebook::fboss::utility
