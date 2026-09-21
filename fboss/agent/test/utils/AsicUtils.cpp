/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/AsicUtils.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss::utility {

const HwAsic* getAsic(const SwSwitch& sw, PortID port) {
  auto switchId = sw.getScopeResolver()->scope(port).switchId();
  return sw.getHwAsicTable()->getHwAsic(switchId);
}

} // namespace facebook::fboss::utility
