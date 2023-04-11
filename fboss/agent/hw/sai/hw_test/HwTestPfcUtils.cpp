/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwTestPfcUtils.h"
#include <gtest/gtest.h>
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace facebook::fboss::utility {

// Gets the PFC enabled/disabled status for RX/TX from HW
void getPfcEnabledStatus(
    const HwSwitch* hw,
    const PortID& portId,
    bool& rxPfc,
    bool& txPfc) {
  const auto& portManager =
      static_cast<const SaiSwitch*>(hw)->managerTable()->portManager();
  auto portHandle = portManager.getPortHandle(portId);
  auto saiPortId = portHandle->port->adapterKey();
  sai_uint8_t txPriorities{0}, rxPriorities{0};

  auto pfcMode = SaiApiTable::getInstance()->portApi().getAttribute(
      saiPortId, SaiPortTraits::Attributes::PriorityFlowControlMode{});
  if (pfcMode == SAI_PORT_PRIORITY_FLOW_CONTROL_MODE_COMBINED) {
    txPriorities = rxPriorities =
        SaiApiTable::getInstance()->portApi().getAttribute(
            saiPortId, SaiPortTraits::Attributes::PriorityFlowControl{});
  } else {
#if !defined(TAJO_SDK)
    rxPriorities = SaiApiTable::getInstance()->portApi().getAttribute(
        saiPortId, SaiPortTraits::Attributes::PriorityFlowControlRx{});
    txPriorities = SaiApiTable::getInstance()->portApi().getAttribute(
        saiPortId, SaiPortTraits::Attributes::PriorityFlowControlTx{});
#endif
  }
  txPfc = txPriorities != 0;
  rxPfc = rxPriorities != 0;
}

// Verifies if the PFC watchdog config provided matches the one
// programmed in BCM HW
void pfcWatchdogProgrammingMatchesConfig(
    const HwSwitch* /* unused */,
    const PortID& /* unused */,
    const bool /* unused */,
    const cfg::PfcWatchdog& /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
}

int getPfcDeadlockDetectionTimerGranularity(int /* unused */) {
  EXPECT_TRUE(false);
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  return 0;
}

int getCosqPFCDeadlockTimerGranularity() {
  EXPECT_TRUE(false);
  return 0;
}

int getProgrammedPfcWatchdogControlParam(
    const HwSwitch* /* unused */,
    const PortID& /* unused */,
    int /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
  return 0;
}

int getPfcWatchdogRecoveryAction() {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
  return -1;
}

// Maps cfg::PfcWatchdogRecoveryAction to SAI specific value
int pfcWatchdogRecoveryAction(cfg::PfcWatchdogRecoveryAction /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
  return 0;
}

void checkSwHwPgCfgMatch(
    const HwSwitch* /*hw*/,
    const std::shared_ptr<Port>& /*swPort*/,
    bool /*pfcEnable*/) {
  // XXX: To be implemented!
}
} // namespace facebook::fboss::utility
