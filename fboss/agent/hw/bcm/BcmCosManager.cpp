/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmCosManager.h"

#include <gflags/gflags.h>

#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

extern "C" {
#include <bcm/cosq.h>
#include <bcm/field.h>
#include <bcm/rx.h>
#include <bcm/switch.h>
#include <bcm/types.h>
}

namespace facebook::fboss {

BcmCosManager::BcmCosManager(BcmSwitch* hw) : hw_(hw) {
  int rv;

  /*
   * By default, ingress field processor is disabled on CPU port. Enable it, as
   * we want the CPU generated packets to go through the ingress pipeline.
   */
  rv = bcm_port_control_set(
      hw->getUnit(), hw_->getCpuGPort(), bcmPortControlFilterIngress, 1);
  bcmCheckError(rv, "failed to enable ingress field processor on CPU port");

  // By default, disables all BST functionalities
  disableBst();
}

/**
 * TODO(joseph5wu) T14668101
 * Should remove this function cause we don't need to get cpu
 * physical port and we should also get cpu gport from hw_->getControlPlane().
 * However, there's some dependency between facebook/BcmPort and
 * facebook/BcmSwitch, which there're some functions of the former needs to
 * call the definition of some functions in the latter, so the linker can pass.
 * If I remove it now, it will fail the build of bcm/facebook/test:bcm_unittest.
 * Will fix it in the future diff.
 */
bcm_port_t BcmCosManager::getPhysicalCpuPort() const {
  return BCM_GPORT_LOCAL_GET(hw_->getCpuGPort());
}

void BcmCosManager::profileSet(
    PortID port,
    int32_t cosq,
    int32_t bid,
    int32_t threshold) {
  bcm_cosq_bst_profile_t profile;
  profile.byte = threshold;
  auto gport = hw_->getPortTable()->getBcmPort(port)->getBcmGport();
  auto rv = bcm_cosq_bst_profile_set(
      hw_->getUnit(), gport, cosq, (bcm_bst_stat_id_t)bid, &profile);
  bcmCheckError(rv, "Failed to set BST profile");
}

int32_t BcmCosManager::profileGet(PortID port, int32_t cosq, int32_t bid) {
  bcm_cosq_bst_profile_t profile;
  auto gport = hw_->getPortTable()->getBcmPort(port)->getBcmGport();
  auto rv = bcm_cosq_bst_profile_get(
      hw_->getUnit(), gport, cosq, (bcm_bst_stat_id_t)bid, &profile);
  bcmCheckError(rv, "Failed to get BST profile");
  return profile.byte;
}

void BcmCosManager::statClear(PortID port, int32_t cosq, int32_t bid) {
  auto gport = hw_->getPortTable()->getBcmPort(port)->getBcmGport();
  auto rv = bcm_cosq_bst_stat_clear(
      hw_->getUnit(), gport, cosq, (bcm_bst_stat_id_t)bid);
  bcmCheckError(rv, "Failed to clear BST stat");
}

uint64_t BcmCosManager::statGet(
    PortID port,
    int32_t cosq,
    int32_t bid,
    bool clearAfter) {
  uint64_t value;
  auto gport = hw_->getPortTable()->getBcmPort(port)->getBcmGport();
  uint32_t options = clearAfter ? BCM_COSQ_STAT_CLEAR : 0;
  auto rv = bcm_cosq_bst_stat_get(
      hw_->getUnit(), gport, cosq, (bcm_bst_stat_id_t)bid, options, &value);
  bcmCheckError(rv, "Failed to get BST stat for port: ", port, " cosq: ", cosq);
  return value;
}

uint64_t BcmCosManager::deviceStatGet(int32_t bid, bool clearAfter) {
  uint64_t value;
  auto rv = bcm_cosq_bst_stat_sync(hw_->getUnit(), (bcm_bst_stat_id_t)bid);
  bcmCheckError(rv, "Failed to sync BST stat");
  uint32_t options = clearAfter ? BCM_COSQ_STAT_CLEAR : 0;
  rv = bcm_cosq_bst_stat_get(
      hw_->getUnit(), 0, 0, (bcm_bst_stat_id_t)bid, options, &value);
  bcmCheckError(rv, "Failed to get BST stat for device");
  return value;
}

void BcmCosManager::deviceStatClear(int32_t bid) {
  auto rv =
      bcm_cosq_bst_stat_clear(hw_->getUnit(), 0, 0, (bcm_bst_stat_id_t)bid);
  bcmCheckError(rv, "Failed to clear BST stat");
}

void BcmCosManager::enableBst() {
  auto rv = bcm_switch_control_set(hw_->getUnit(), bcmSwitchBstEnable, 1);
  bcmCheckError(rv, "failed to enable BST");
  // BRCM include/bcm/switch.h is wrong, it defines
  // bcmSwitchBstTrackPeak = 0
  // However downstream code expects tracks peak using
  // if (track_mode), expecting a non zero value here.
  constexpr auto kTrackPeak = 1;
  rv = bcm_switch_control_set(
      hw_->getUnit(), bcmSwitchBstTrackingMode, kTrackPeak);
  bcmCheckError(rv, "failed to setup BST tracking mode");
  rv = bcm_switch_control_set(hw_->getUnit(), bcmSwitchBstSnapshotEnable, 0);
  bcmCheckError(rv, "failed to disable BST snapshot mode");
}

void BcmCosManager::disableBst() {
  auto rv = 0;

  rv = bcm_switch_control_set(hw_->getUnit(), bcmSwitchBstTrackingMode, 0);
  bcmCheckError(rv, "failed to clear BST tracking mode");

  rv = bcm_switch_control_set(hw_->getUnit(), bcmSwitchBstEnable, 0);
  bcmCheckError(rv, "failed to disable BST");
}
} // namespace facebook::fboss
