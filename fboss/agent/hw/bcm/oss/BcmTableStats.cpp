/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTableStats.h"

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

extern "C" {
#include <bcm/switch.h>
}

namespace facebook::fboss {

int BcmHwTableStatManager::getAlpmMaxRouteCount(int& count, bool isV6) const {
  if (!isV6) {
    return bcm_switch_object_count_get(
        0, bcmSwitchObjectL3RouteV4RoutesMax, &count);
  }
  if (is128ByteIpv6Enabled_) {
    return bcm_switch_object_count_get(
        0, bcmSwitchObjectL3RouteV6Routes128bMax, &count);
  }
  return bcm_switch_object_count_get(
      0, bcmSwitchObjectL3RouteV6Routes64bMax, &count);
}

int BcmHwTableStatManager::getAlpmUsedRouteCount(int& count, bool isV6) const {
  if (!isV6) {
    return bcm_switch_object_count_get(
        0, bcmSwitchObjectL3RouteV4RoutesUsed, &count);
  }
  if (is128ByteIpv6Enabled_) {
    return bcm_switch_object_count_get(
        0, bcmSwitchObjectL3RouteV6Routes128bUsed, &count);
  }
  return bcm_switch_object_count_get(
      0, bcmSwitchObjectL3RouteV6Routes64bUsed, &count);
}

} // namespace facebook::fboss
