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
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include "fboss/agent/HwSwitch.h"

#include <folly/IPAddress.h>

namespace facebook::fboss::utility {

std::optional<cfg::AclLookupClass> getHwRouteClassID(
    HwSwitch* hwSwitch,
    folly::CIDRNetwork cidrNetwork);

} // namespace facebook::fboss::utility
