/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwVlanUtils.h"

namespace facebook {
namespace fboss {

std::vector<VlanID> getConfiguredVlans(const HwSwitch* hwSwitch) {
  return {};
}

std::map<VlanID, uint32_t> getVlanToNumPorts(const HwSwitch* hwSwitch) {
  return {};
}
} // namespace fboss
} // namespace facebook
