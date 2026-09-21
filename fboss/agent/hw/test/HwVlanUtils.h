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

#include "fboss/agent/types.h"

#include <map>
#include <vector>

namespace facebook::fboss {

class HwSwitch;
/*
 * APIs to query VLAN information off the underlying HW. Each
 * distinct HW (SAI, BRCM) implementation is responsible for
 * providing a implementation for these APIs
 */

std::vector<VlanID> getConfiguredVlans(const HwSwitch* hwSwitch);

std::map<VlanID, uint32_t> getVlanToNumPorts(const HwSwitch* hwSwitch);

} // namespace facebook::fboss
