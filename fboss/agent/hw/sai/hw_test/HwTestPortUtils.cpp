/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestPortUtils.h"

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiPortUtils.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include <algorithm>

namespace facebook::fboss::utility {

void setPortLoopbackMode(
    const HwSwitch* hwSwitch,
    PortID portId,
    cfg::PortLoopbackMode lbMode) {
  // Use concurrent indices to make this thread safe
  const auto& portMapping =
      static_cast<const SaiSwitch*>(hwSwitch)->concurrentIndices().portIds;
  // ConcurrentHashMap deletes copy constructor for its iterators, making it
  // impossible to use a std::find_if here. So rollout a ugly hand written loop
  auto portItr = portMapping.begin();
  for (; portItr != portMapping.end(); ++portItr) {
    if (portItr->second == portId) {
      break;
    }
  }
  CHECK(portItr != portMapping.end());
  SaiPortTraits::Attributes::InternalLoopbackMode internalLbMode{
      utility::getSaiPortInternalLoopbackMode(lbMode)};
  auto& portApi = SaiApiTable::getInstance()->portApi();
  portApi.setAttribute(portItr->first, internalLbMode);
}
} // namespace facebook::fboss::utility
