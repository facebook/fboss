/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwPortUtils.h"

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss::utility {

namespace {
SaiPortTraits::AdapterKey getPortAdapterKey(const HwSwitch* hw, PortID port) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto handle = saiSwitch->managerTable()->portManager().getPortHandle(port);
  CHECK(handle);
  return handle->port->adapterKey();
}

} // namespace
bool portEnabled(const HwSwitch* hw, PortID port) {
  auto key = getPortAdapterKey(hw, port);
  SaiPortTraits::Attributes::AdminState state;
  SaiApiTable::getInstance()->portApi().getAttribute(key, state);
  return state.value();
}

cfg::PortSpeed curPortSpeed(const HwSwitch* hw, PortID port) {
  auto key = getPortAdapterKey(hw, port);
  SaiPortTraits::Attributes::Speed speed;
  SaiApiTable::getInstance()->portApi().getAttribute(key, speed);
  return cfg::PortSpeed(speed.value());
}

void assertPort(
    const HwSwitch* hw,
    PortID port,
    bool enabled,
    cfg::PortSpeed speed) {
  CHECK_EQ(enabled, portEnabled(hw, port));
  if (enabled) {
    // Only verify speed on enabled ports
    CHECK_EQ(
        static_cast<int>(speed),
        static_cast<int>(utility::curPortSpeed(hw, port)));
  }
}

void assertPortStatus(const HwSwitch* hw, PortID port) {
  CHECK(portEnabled(hw, port));
}

void assertPortsLoopbackMode(
    const HwSwitch* hw,
    const std::map<PortID, int>& port2LoopbackMode) {
  for (auto portAndLoopBackMode : port2LoopbackMode) {
    assertPortLoopbackMode(
        hw, portAndLoopBackMode.first, portAndLoopBackMode.second);
  }
}

void assertPortSampleDestination(
    const HwSwitch* /*hw*/,
    PortID /*port*/,
    int /*expectedSampleDestination*/) {
  throw FbossError("sampling is unsupported for SAI");
}

void assertPortsSampleDestination(
    const HwSwitch* /*hw*/,
    const std::map<PortID, int>& /*port2SampleDestination*/) {
  throw FbossError("sampling is unsupported for SAI");
}

void assertPortLoopbackMode(
    const HwSwitch* hw,
    PortID port,
    int expectedLoopbackMode) {
  auto key = getPortAdapterKey(hw, port);
  SaiPortTraits::Attributes::InternalLoopbackMode loopbackMode;
  SaiApiTable::getInstance()->portApi().getAttribute(key, loopbackMode);
  CHECK_EQ(expectedLoopbackMode, loopbackMode.value());
}

} // namespace facebook::fboss::utility
