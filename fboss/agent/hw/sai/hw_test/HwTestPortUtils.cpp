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
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortUtils.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/platforms/sai/SaiPlatformPort.h"

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
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  SaiPortTraits::Attributes::PortLoopbackMode portLoopbackMode{
      utility::getSaiPortLoopbackMode(lbMode)};
  auto& portApi = SaiApiTable::getInstance()->portApi();
  portApi.setAttribute(portItr->first, portLoopbackMode);
#else
  SaiPortTraits::Attributes::InternalLoopbackMode internalLbMode{
      utility::getSaiPortInternalLoopbackMode(lbMode)};
  auto& portApi = SaiApiTable::getInstance()->portApi();
  portApi.setAttribute(portItr->first, internalLbMode);
#endif
}

void setPortTxEnable(const HwSwitch* hw, PortID port, bool enable) {
  auto portHandle = static_cast<const SaiSwitch*>(hw)
                        ->managerTable()
                        ->portManager()
                        .getPortHandle(port);

  portHandle->port->setOptionalAttribute(
      SaiPortTraits::Attributes::PktTxEnable{enable});
  auto switchID = static_cast<const SaiSwitch*>(hw)->getSaiSwitchId();
  // Disable credit WD for VoQ switches to avoid drop in queue when TX is
  // disabled!
  if (hw->getPlatform()->getAsic()->getSwitchType() == cfg::SwitchType::VOQ) {
    SaiApiTable::getInstance()->switchApi().setAttribute(
        switchID, SaiSwitchTraits::Attributes::CreditWd{enable});
  }
}

void enableTransceiverProgramming(bool enable) {
  FLAGS_skip_transceiver_programming = !enable;
}
} // namespace facebook::fboss::utility
