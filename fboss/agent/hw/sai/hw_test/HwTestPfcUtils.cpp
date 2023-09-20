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

// Maps PFC DLR _sai_packet_action_t to cfg::PfcWatchdogRecoveryAction
cfg::PfcWatchdogRecoveryAction pfcWatchdogRecoveryAction(
    int pfcDlrPacketAction) {
  cfg::PfcWatchdogRecoveryAction recoveryAction;
  if (pfcDlrPacketAction == SAI_PACKET_ACTION_DROP) {
    recoveryAction = cfg::PfcWatchdogRecoveryAction::DROP;
  } else {
    recoveryAction = cfg::PfcWatchdogRecoveryAction::NO_DROP;
  }
  return recoveryAction;
}

std::optional<cfg::PfcWatchdog> getProgrammedPfcDeadlockParams(
    const HwSwitch* hw,
    const PortID& portId) {
  auto portHandle = static_cast<const SaiSwitch*>(hw)
                        ->managerTable()
                        ->portManager()
                        .getPortHandle(portId);
  CHECK(portHandle);
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
  std::vector<sai_map_t> pfcDldTimer =
      SaiApiTable::getInstance()->portApi().getAttribute(
          portHandle->port->adapterKey(),
          SaiPortTraits::Attributes::PfcTcDldInterval{});
  std::vector<sai_map_t> pfcDlrTimer =
      SaiApiTable::getInstance()->portApi().getAttribute(
          portHandle->port->adapterKey(),
          SaiPortTraits::Attributes::PfcTcDlrInterval{});
  // PfcWD is programmed in HW, we'll see non zero
  // PFC DLD/DLR timers.
  if (pfcDldTimer.size() && pfcDlrTimer.size()) {
    cfg::PfcWatchdog pfcWd;
    pfcWd.detectionTimeMsecs() = pfcDlrTimer.at(0).value;
    // Timers for all priorities should be the same!
    for (const auto& dldTimer : pfcDldTimer) {
      EXPECT_EQ(dldTimer.value, *pfcWd.detectionTimeMsecs());
    }
    pfcWd.recoveryTimeMsecs() = pfcDlrTimer.at(0).value;
    // Timers for all priorities should be the same!
    for (const auto& dlrTimer : pfcDlrTimer) {
      EXPECT_EQ(dlrTimer.value, *pfcWd.recoveryTimeMsecs());
    }

    auto pfcDlrPacketAction =
        SaiApiTable::getInstance()->switchApi().getAttribute(
            static_cast<const SaiSwitch*>(hw)->getSaiSwitchId(),
            SaiSwitchTraits::Attributes::PfcDlrPacketAction{});
    pfcWd.recoveryAction() = pfcWatchdogRecoveryAction(pfcDlrPacketAction);
    return pfcWd;
  }
#endif
  return std::nullopt;
}

// Verifies if the PFC watchdog config provided matches the one
// programmed in HW
void pfcWatchdogProgrammingMatchesConfig(
    const HwSwitch* hw,
    const PortID& portId,
    const bool watchdogEnabled,
    const cfg::PfcWatchdog& watchdog) {
  auto pfcWdProgrammed = getProgrammedPfcDeadlockParams(hw, portId);
  EXPECT_EQ(watchdogEnabled, pfcWdProgrammed.has_value());
  if (pfcWdProgrammed.has_value()) {
    EXPECT_EQ(watchdog, *pfcWdProgrammed);
  }
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

cfg::PfcWatchdogRecoveryAction getPfcWatchdogRecoveryAction(
    const HwSwitch* hw,
    const PortID& portId) {
  auto pfcWdProgrammed = getProgrammedPfcDeadlockParams(hw, portId);
  if (pfcWdProgrammed.has_value()) {
    return *pfcWdProgrammed->recoveryAction();
  }
  throw FbossError("PFC Watchdog is not configured!");
}

void checkSwHwPgCfgMatch(
    const HwSwitch* hw,
    const std::shared_ptr<Port>& swPort,
    bool pfcEnable) {
  auto swPgConfig = swPort->getPortPgConfigs();

  auto portHandle = static_cast<const SaiSwitch*>(hw)
                        ->managerTable()
                        ->portManager()
                        .getPortHandle(PortID(swPort->getID()));
  for (const auto& pgConfig : std::as_const(*swPgConfig)) {
    auto id = pgConfig->cref<switch_state_tags::id>()->cref();
    auto iter = portHandle->configuredIngressPriorityGroups.find(
        static_cast<IngressPriorityGroupID>(id));
    if (iter == portHandle->configuredIngressPriorityGroups.end()) {
      throw FbossError(
          "Priority group config canot be found for PG id ",
          id,
          " on port ",
          swPort->getName());
    }
    auto bufferProfile = iter->second.bufferProfile;
    EXPECT_EQ(
        pgConfig->cref<switch_state_tags::resumeOffsetBytes>()->cref(),
        SaiApiTable::getInstance()->bufferApi().getAttribute(
            bufferProfile->adapterKey(),
            SaiBufferProfileTraits::Attributes::XonOffsetTh{}));
    EXPECT_EQ(
        pgConfig->cref<switch_state_tags::minLimitBytes>()->cref(),
        SaiApiTable::getInstance()->bufferApi().getAttribute(
            bufferProfile->adapterKey(),
            SaiBufferProfileTraits::Attributes::ReservedBytes{}));
    EXPECT_EQ(
        pgConfig->cref<switch_state_tags::minLimitBytes>()->cref(),
        SaiApiTable::getInstance()->bufferApi().getAttribute(
            bufferProfile->adapterKey(),
            SaiBufferProfileTraits::Attributes::ReservedBytes{}));
    if (auto pgHdrmOpt =
            pgConfig->cref<switch_state_tags::headroomLimitBytes>()) {
      EXPECT_EQ(
          pgHdrmOpt->cref(),
          SaiApiTable::getInstance()->bufferApi().getAttribute(
              bufferProfile->adapterKey(),
              SaiBufferProfileTraits::Attributes::XoffTh{}));
    }

    // Buffer pool configs
    const auto bufferPool =
        pgConfig->cref<switch_state_tags::bufferPoolConfig>();
    EXPECT_EQ(
        bufferPool->cref<switch_state_tags::headroomBytes>()->cref() *
            static_cast<const SaiSwitch*>(hw)
                ->getPlatform()
                ->getAsic()
                ->getNumMemoryBuffers(),
        SaiApiTable::getInstance()->bufferApi().getAttribute(
            static_cast<const SaiSwitch*>(hw)
                ->managerTable()
                ->bufferManager()
                .getIngressBufferPoolHandle()
                ->bufferPool->adapterKey(),
            SaiBufferPoolTraits::Attributes::XoffSize{}));

    // Port PFC configurations
    if (SaiApiTable::getInstance()->portApi().getAttribute(
            portHandle->port->adapterKey(),
            SaiPortTraits::Attributes::PriorityFlowControlMode{}) ==
        SAI_PORT_PRIORITY_FLOW_CONTROL_MODE_COMBINED) {
      EXPECT_EQ(
          SaiApiTable::getInstance()->portApi().getAttribute(
              portHandle->port->adapterKey(),
              SaiPortTraits::Attributes::PriorityFlowControl{}) != 0,
          pfcEnable);
    } else {
#if !defined(TAJO_SDK)
      EXPECT_EQ(
          SaiApiTable::getInstance()->portApi().getAttribute(
              portHandle->port->adapterKey(),
              SaiPortTraits::Attributes::PriorityFlowControlTx{}) != 0,
          pfcEnable);
#else
      throw FbossError("Flow control mode SEPARATE unsupported!");
#endif
    }
  }
}

void checkSwHwPgIdSetMatch(
    const HwSwitch* /* unused */,
    const std::shared_ptr<Port>& /* unused */,
    std::set<int>& /* unused */) {
  EXPECT_TRUE(false);
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
}
} // namespace facebook::fboss::utility
