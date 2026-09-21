/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestNeighborUtils.h"

#include "fboss/agent/hw/sai/api/NeighborApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiNeighborManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

namespace utility {

bool isHostHit(const HwSwitch* /*hwSwitch*/, const folly::IPAddress& /*ip*/) {
  throw FbossError("Neighbor entry hitbit is unsupported for SAI");
}

void clearHostHitBit(
    const HwSwitch* /*hwSwitch*/,
    const folly::IPAddress& /*ip*/) {
  throw FbossError("Neighbor entry hitbit is unsupported for SAI");
}

} // namespace utility
} // namespace facebook::fboss
