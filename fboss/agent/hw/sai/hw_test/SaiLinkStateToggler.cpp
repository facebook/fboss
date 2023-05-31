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
#include "fboss/agent/test/TestEnsembleIf.h"

namespace facebook::fboss {

void SaiLinkStateToggler::setPortPreemphasis(
    const std::shared_ptr<Port>& port,
    int preemphasis) {
  auto& portManager = getHw()->managerTable()->portManager();
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

  auto setTxRxAttr = [](auto& attrs, auto type, const auto& val) {
    auto& attr = std::get<std::optional<std::decay_t<decltype(type)>>>(attrs);
    if (!val.empty()) {
      attr = val;
    }
  };
  auto preemphasisVal =
      std::vector<uint32_t>(numLanes, static_cast<uint32_t>(preemphasis));
  if (getHw()->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::SAI_PORT_SERDES_FIELDS_RESET)) {
    setTxRxAttr(
        serDesAttributes,
        SaiPortSerdesTraits::Attributes::Preemphasis{},
        preemphasisVal);
  } else if (preemphasis != 0) {
    // set different txfir only when input value is not zero,
    // otherwise use default settings from serdesAttributesFromSwPinConfigs()
    setTxRxAttr(
        serDesAttributes,
        SaiPortSerdesTraits::Attributes::TxFirPre1{},
        preemphasisVal);
    setTxRxAttr(
        serDesAttributes,
        SaiPortSerdesTraits::Attributes::TxFirPost1{},
        preemphasisVal);
    setTxRxAttr(
        serDesAttributes,
        SaiPortSerdesTraits::Attributes::TxFirMain{},
        preemphasisVal);
  }
  if (getHw()->getPlatform()->isSerdesApiSupported()) {
    portHandle->serdes->setAttributes(serDesAttributes);
  }
}

void SaiLinkStateToggler::setLinkTraining(
    const std::shared_ptr<Port>& port,
    bool enable) {
  auto& portManager = getHw()->managerTable()->portManager();
  auto portHandle = portManager.getPortHandle(port->getID());
  if (!portHandle) {
    throw FbossError(
        "Cannot set link training on non existent port: ", port->getID());
  }

  portHandle->port->setOptionalAttribute(
      SaiPortTraits::Attributes::LinkTrainingEnable{enable});
}

void SaiLinkStateToggler::setRxLaneSquelchImpl(
    const std::shared_ptr<Port>& port,
    bool enable) {
  auto& portManager = getHw()->managerTable()->portManager();
  auto portHandle = portManager.getPortHandle(port->getID());
  if (!portHandle) {
    throw FbossError(
        "Cannot set Rx Lane Squelch on non existent port: ", port->getID());
  }

  portHandle->port->setOptionalAttribute(
      SaiPortTraits::Attributes::RxLaneSquelchEnable{enable});
}

SaiSwitch* SaiLinkStateToggler::getHw() const {
  return static_cast<SaiSwitch*>(getHwSwitchEnsemble()->getHwSwitch());
}

std::unique_ptr<HwLinkStateToggler> createHwLinkStateToggler(
    TestEnsembleIf* ensemble,
    const std::map<cfg::PortType, cfg::PortLoopbackMode>&
        desiredLoopbackModes) {
  return std::make_unique<SaiLinkStateToggler>(ensemble, desiredLoopbackModes);
}
} // namespace facebook::fboss
