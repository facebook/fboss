/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/hw_test/SaiLinkStateToggler.h"

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace facebook::fboss {
void SaiLinkStateToggler::setPortPreemphasis(
    const std::shared_ptr<Port>& port,
    int preemphasis) {
  auto& portManager = static_cast<SaiSwitchEnsemble*>(getHwSwitchEnsemble())
                          ->getHwSwitch()
                          ->managerTable()
                          ->portManager();
  auto portHandle = portManager.getPortHandle(port->getID());
  if (!portHandle) {
    throw FbossError(
        "Cannot set preemphasis on non existent port: ", port->getID());
  }
  auto gotAttributes = portHandle->port->attributes();
  auto numLanes = std::get<SaiPortTraits::Attributes::HwLaneList>(gotAttributes)
                      .value()
                      .size();

  auto portSaiId = portHandle->port->adapterKey();
  auto serDesAttributes = portManager.serdesAttributesFromSwPinConfigs(
      portSaiId, port->getPinConfigs(), portHandle->serdes);

  std::get<std::optional<SaiPortSerdesTraits::Attributes::Preemphasis>>(
      serDesAttributes) =
      std::vector<uint32_t>(numLanes, static_cast<uint32_t>(preemphasis));

  if (saiEnsemble_->getPlatform()->isSerdesApiSupported()) {
    portHandle->serdes->setAttributes(serDesAttributes);
  }
}
} // namespace facebook::fboss
