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

#include "fboss/agent/HwSwitch.h"

namespace facebook::fboss::utility {
// TODO (skhare)
// When SwitchState is extended to carry AclTable, pass AclTable handle to
// getAclTableNumAclEntries.
int getAclTableNumAclEntries(const HwSwitch* hwSwitch);

void checkSwHwAclMatch(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName);

} // namespace facebook::fboss::utility
