/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmSwitchSettings.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmPtpTcMgr.h"
#include "fboss/agent/hw/bcm/BcmQcmManager.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

extern "C" {
#include <bcm/port.h>
#include <bcm/switch.h>
}

namespace facebook::fboss {

void BcmSwitchSettings::setL2LearningMode(cfg::L2LearningMode l2LearningMode) {
  switch (l2LearningMode) {
    case cfg::L2LearningMode::HARDWARE:
      enableL2LearningHardware();
      break;
    case cfg::L2LearningMode::SOFTWARE:
      enableL2LearningSoftware();
      break;
  }

  l2LearningMode_ = l2LearningMode;
}

void BcmSwitchSettings::enableL2LearningHardware() {
  if (l2LearningMode_.has_value() &&
      l2LearningMode_.value() == cfg::L2LearningMode::HARDWARE) {
    return;
  }

  disableL2LearningCallback();
  disablePendingEntriesOnUnknownSrcL2();

  /*
   * For every L2 entry previously learned in SOFTWARE mode, remove it from
   * SwitchState's MAC Table.
   * This is achieved by traversing L2 table, and generating L2 table update
   * DELETE callback for each entry in L2 table.
   *
   * At this stage, L2 table update callbacks are disabled, so we don't need to
   * worry about more entries getting added into SwitchState's MAC Table.
   *
   * However, this results into BcmSwitch::processMacTableChanges receiving a
   * stateDelta with all L2 addresses removed. This would cause unknown unicast
   * flood till the L2 entries are relearned. To avoid,
   * BcmSwitch::processMacTableChanges is no-op when in
   * l2LearningMode::HARDWARE>
   */
  auto rv = bcm_l2_traverse(hw_->getUnit(), BcmSwitch::deleteL2TableCb, hw_);
  bcmCheckError(rv, "bcm_l2_traverse failed");
}

void BcmSwitchSettings::enableL2LearningSoftware() {
  if (l2LearningMode_.has_value() &&
      l2LearningMode_.value() == cfg::L2LearningMode::SOFTWARE) {
    return;
  }

  /*
   * Graceful warmboot exit unregisters the callback. We must register the
   * callback again post warmboot even if the mode currently programmed in ASIC
   * is Software Learning.
   */
  enableL2LearningCallback();

  /*
   * If the mode is Software learning mode prior to warmboot, do nothing: ASIC
   * is already programmed correctly and switch state's MAC table will be
   * populated from the warmboot state.
   * Otherwise, program the ASIC and update switch state's MAC table.
   */

  if (hw_->getWarmBootCache()->getL2LearningMode() !=
      cfg::L2LearningMode::SOFTWARE) {
    enablePendingEntriesOnUnknownSrcL2();
    /*
     * For every L2 entry previously learned in HARDWARE mode, populate
     * SwitchState's MAC Table.
     * This is achieved by traversing L2 table, and generating L2 table update
     * ADD callback for each entry in L2 table.
     *
     * We don't need to worry about MACs learned after this traverse as L2 table
     * update callbacks are already enabled. However, this also means that we
     * will end up delivering duplicate callbacks for L2 entries learned in the
     * time window between enabling L2 table update callback and traversal. This
     * is fine given how our implementation handles it: a call to add macEntry
     * that exists is a no-op.
     */
    auto rv = bcm_l2_traverse(hw_->getUnit(), BcmSwitch::addL2TableCb, hw_);
    bcmCheckError(rv, "bcm_l2_traverse failed");
  }
}

void BcmSwitchSettings::enableL2LearningCallback() {
  auto unit = hw_->getUnit();

  /*
   * For TH3/TH4 below API is not supported/needed. Registering L2 learning
   * callback is sufficient.
   */
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::PENDING_L2_ENTRY)) {
    /*
     * Configure callback for L2 table update events, and configure the udpate
     * events to generate callback for.
     *
     * Note:
     *   Until BCM SDK 6.5.24, callback for L2 CPU Add Event was also
     *   subscribed. But due to a bug in BCM SDK, callback for L2 CPU Add
     *   Event was not received at all. This got fixed in BCM SDK 6.5.24,
     *   resulting in an infinite loop after learn callback event - CPU
     *   Add -> CPU Add Callback -> CPU Delete -> CPU Delete Callback ->
     *   CPU Add -> CPU Add Callback and so on. So, as per Broadcom's
     *   suggestion, from BCM SDK 6.5.24 onwards, FBOSS stopped subscribing
     *   to L2 CPU Add Event as there is no use case for this in FBOSS.
     *
     *   In case there arises a need for subscribing to L2 Add Event, then
     *   callback for FBOSS' own CPU Add should be disabled for each MAC add
     *   (by setting the appropriate flag for this during CPU add API call).
     */
    auto rv = bcm_switch_control_set(unit, bcmSwitchL2LearnEvent, 1);
    bcmCheckError(rv, "Failed to subscribe to L2 CPU Learn Event");
    rv = bcm_switch_control_set(unit, bcmSwitchL2AgingEvent, 1);
    bcmCheckError(rv, "Failed to subscribe to L2 CPU Aging Event");
  }

  auto rv = bcm_l2_addr_register(
      unit,
      BcmSwitch::l2LearningCallback, // bcm_l2_addr_callback_t
      hw_); // void *userdata
  bcmCheckError(rv, "Failed to register l2 addr callback");

  l2AddrCallBackRegisterd_ = true;
}

void BcmSwitchSettings::disableL2LearningCallback() {
  /*
   * If callback was not registered, there is nothing to unregister (this could
   * happen if agent comes up with HARDWARE learning.
   */
  if (!l2AddrCallBackRegisterd_) {
    return;
  }

  auto unit = hw_->getUnit();

  /*
   * For TH3/TH4, below API is not supported/needed. Registering L2 learning
   * callback is sufficient.
   */
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::PENDING_L2_ENTRY)) {
    /*
     * Unconfigure callback for L2 table update events.
     */
    auto rv = bcm_switch_control_set(unit, bcmSwitchL2LearnEvent, 0);
    bcmCheckError(rv, "Failed to unsubscribe from L2 CPU Learn Event");
    rv = bcm_switch_control_set(unit, bcmSwitchL2AgingEvent, 0);
    bcmCheckError(rv, "Failed to unsubscribe from L2 CPU Aging Event");
  }

  auto rv = bcm_l2_addr_unregister(
      unit,
      BcmSwitch::l2LearningCallback, // bcm_l2_addr_callback_t
      hw_); // void *userdata
  bcmCheckError(rv, "Failed to unregister l2 addr callback");

  l2AddrCallBackRegisterd_ = false;
}

void BcmSwitchSettings::enablePendingEntriesOnUnknownSrcL2() {
  /*
   * Disables L2 Learning in hardware for every port.
   * Furthermore, when a packet with unknown source MAC + vlan is seen, the
   * hardware would added such MAC + vlan to L2 table but mark it as PENDING.
   * Packets with PENDING as destination are treated as unknown destination
   * (and flooded) till the software explicitly "validates" the PENDING entry
   * via call to bcm_l2_addr_add. Subsequent packets with same source MAC +
   * vlan find that the entry exists in the L2 table (PENDING or validated) and
   * do not generate further callbacks or traps to CPU.
   * (We don't pass BCM_PORT_LEARN_CPU flag, so the packets would not have
   * trapped to CPU anyway).
   *
   * For TH3/TH4, there are no PENDING entries, and thus below API is not
   * supported/needed.
   */
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::PENDING_L2_ENTRY)) {
    auto unit = hw_->getUnit();
    for (const auto& portIDAndBcmPort : *hw_->getPortTable()) {
      auto portID = portIDAndBcmPort.first;
      auto rv = bcm_port_learn_set(
          unit, portID, BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_PENDING);
      bcmCheckError(
          rv, "Failed to enable software learning for port: ", portID);
    }
  }
}

void BcmSwitchSettings::disablePendingEntriesOnUnknownSrcL2() {
  /*
   * Enable L2 Learning in hardware for every port.
   * This is usual hardware L2 learning: packets with unknown source MAC + vlan
   * result into new entry added to L2 table for source MAC + vlan with
   * srcPort. Packets to this L2 entry are switched directly to the port in the
   * L2 entry (no flooded). No traps to CPU on such packets.
   *
   * For TH3/TH4, there are no PENDING entries, and thus below API is not
   * supported/needed.
   */
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::PENDING_L2_ENTRY)) {
    auto unit = hw_->getUnit();
    for (const auto& portIDAndBcmPort : *hw_->getPortTable()) {
      auto portID = portIDAndBcmPort.first;
      auto rv = bcm_port_learn_set(
          unit, portID, BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_FWD);
      bcmCheckError(
          rv, "Failed to disable software learning for port: ", portID);
    }
  }
}

void BcmSwitchSettings::setQcmEnable(
    bool qcmEnable,
    const std::shared_ptr<SwitchState>& swState) {
  XLOG(DBG3) << "Set qcm =" << qcmEnable;
  if (qcmEnable_.has_value() && qcmEnable_.value() == qcmEnable) {
    return;
  }

  const auto bcmQcmMgrPtr = hw_->getBcmQcmMgr();
  qcmEnable ? bcmQcmMgrPtr->init(swState) : bcmQcmMgrPtr->stop();
  qcmEnable_ = qcmEnable;
}

void BcmSwitchSettings::setPtpTc(
    bool enable,
    const std::shared_ptr<SwitchState>& /* unused */) {
  if (ptpTcEnable_.has_value()) {
    if (ptpTcEnable_.value() == enable) {
      XLOG(DBG3) << "Skip setting ptp tc = " << enable
                 << " as no-op based on previous config";
      return;
    }
  } else {
    if (hw_->getWarmBootCache()->ptpTcSettingMatches(enable)) {
      XLOG(DBG3) << "Skip setting ptp tc = " << enable
                 << " as no-op based on warmboot cache";
      ptpTcEnable_ = enable;
      return;
    }
  }

  XLOG(DBG3) << "Set ptp tc =" << enable;

  const auto ptpTcMgr = hw_->getPtpTcMgr();
  (enable) ? ptpTcMgr->enablePtpTc() : ptpTcMgr->disablePtpTc();

  ptpTcEnable_ = enable;
  hw_->getWarmBootCache()->ptpTcProgrammed();
}

void BcmSwitchSettings::setL2AgeTimerSeconds(uint32_t val) {
  XLOG(DBG3) << "Set l2AgeTimerSeconds =" << val;
  l2AgeTimerSeconds_ = val;
}
} // namespace facebook::fboss
