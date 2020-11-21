/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmPtpTcMgr.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/state/SwitchState.h"

extern "C" {
#include <bcm/switch.h>
#include <bcm/time.h>
}

namespace facebook::fboss {

int BcmPtpTcMgr::getTsBitModeArg(HwAsic::AsicType asicType) {
  // TODO(rajukumarfb5368): integrate with HwAsic::isSupported() or a variant
  switch (asicType) {
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
      return bcmTimesyncTimestampingMode32bit;
    case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
      return bcmTimesyncTimestampingMode48bit;
    default:
      throw FbossError(
          "Device type ",
          asicType,
          " not supported yet, please only use TH or TH3");
  }

  return -1;
}

void BcmPtpTcMgr::enablePortTimesyncConfig(
    bcm_port_timesync_config_t* port_timesync_config) {
  /* Set port timesync configuration struct flags to 1-step TC and mandatory
   * update of all packets' correction field (CF) */
  port_timesync_config->flags |= BCM_PORT_TIMESYNC_DEFAULT;
  port_timesync_config->flags |=
      BCM_PORT_TIMESYNC_ONE_STEP_TIMESTAMP; /* Set to OSTS - One-Step
                                               TimeStamping */
  port_timesync_config->flags |=
      BCM_PORT_TIMESYNC_TIMESTAMP_CFUPDATE_ALL; /* Always update correction
                                                   field */
}

void BcmPtpTcMgr::enablePcsBasedTimestamping(bcm_port_t port) {
  const auto unit = hw_->getUnit();

  // TODO(rajukumarfb5368): Use bcm_port_control_phy_timesync_set() in SDK
  // > 6.5.18 to configure timestamp offset and adjust. Ref:
  // https://www.internalfb.com/diff/D24411684 (Base vs V11)

  bcm_port_phy_timesync_config_t phy;
  bcm_port_phy_timesync_config_t_init(&phy);
  phy.flags = BCM_PORT_PHY_TIMESYNC_ENABLE; /* Enable PHY based timesync */
  phy.flags |= BCM_PORT_PHY_TIMESYNC_ONE_STEP_ENABLE; /* Set PHY timesync to
                                                         one-step TS */

  BCM_CHECK_ERROR(
      bcm_port_phy_timesync_config_set(unit, port, &phy),
      "failed to enable TC on port ",
      port);
}

void BcmPtpTcMgr::disablePcsBasedTimestamping(bcm_port_t port) {
  const auto unit = hw_->getUnit();

  bcm_port_phy_timesync_config_t phy;
  bcm_port_phy_timesync_config_t_init(&phy);

  BCM_CHECK_ERROR(
      bcm_port_phy_timesync_config_set(unit, port, &phy),
      "failed to disable TC on port ",
      port);
}

void BcmPtpTcMgr::enableTimeInterface() {
  bcm_time_interface_t time_interface; /* device time (abstract) interface
                                          configuration struct */
  bcm_time_interface_t_init(&time_interface);
  time_interface.flags = BCM_TIME_ENABLE;
  bcm_time_interface_add(hw_->getUnit(), &time_interface);
}

void BcmPtpTcMgr::disableTimeInterface() {
  bcm_time_interface_delete_all(hw_->getUnit());
}

bool BcmPtpTcMgr::isPtpTcSupported() {
  return hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::PTP_TC);
}

bool BcmPtpTcMgr::isPtpTcPcsSupported() {
  return hw_->getPlatform()->getAsic()->isSupported(
      HwAsic::Feature::PTP_TC_PCS);
}

/* separate configuration for MAC based timestamping devices - TD2/TH, and
 * PCS based timestamping devices - TH3 */
void BcmPtpTcMgr::enablePtpTc() {
  if (!isPtpTcSupported()) {
    XLOG(INFO) << "[PTP] Ignore configuration of unsupported feature";
    return;
  }

  XLOG(INFO) << "[PTP] Start enable";

  const auto unit = hw_->getUnit();

  bcm_port_timesync_config_t port_timesync_config;
  bcm_port_timesync_config_t_init(&port_timesync_config);
  enablePortTimesyncConfig(&port_timesync_config);

  bcm_port_config_t portConfig;
  bcm_port_config_t_init(&portConfig);
  BCM_CHECK_ERROR(
      bcm_port_config_get(unit, &portConfig),
      "failed to get port configuration");

  int count = 0;
  bcm_port_t port;
  BCM_PBMP_ITER(portConfig.port, port) {
    if (isPtpTcPcsSupported()) {
      enablePcsBasedTimestamping(port);
    }

    // TODO(rajukumarfb5368): high level description of bcm_*() calls needed

    BCM_CHECK_ERROR(
        bcm_port_timesync_config_set(unit, port, 1, &port_timesync_config),
        "Error in enabling TC correction for port ",
        port);

    static const int tsBitModeArg =
        getTsBitModeArg(hw_->getPlatform()->getAsic()->getAsicType());
    BCM_CHECK_ERROR(
        bcm_switch_control_port_set(
            unit, port, bcmSwitchTimesyncEgressTimestampingMode, tsBitModeArg),
        "Error in enabling TC correction for port ",
        port);

    count++;
  }

  enableTimeInterface();
  XLOG(INFO) << "[PTP] Enabled timestamping on " << count << " ports";
}

void BcmPtpTcMgr::disablePtpTc() {
  if (!isPtpTcSupported()) {
    XLOG(INFO) << "[PTP] Ignore configuration of unsupported feature";
    return;
  }

  XLOG(INFO) << "[PTP] Start disable";

  const auto unit = hw_->getUnit();
  bcm_port_config_t portConfig;
  bcm_port_config_t_init(&portConfig);
  BCM_CHECK_ERROR(
      bcm_port_config_get(unit, &portConfig),
      "failed to get port configuration");

  int count = 0;
  bcm_port_t port;
  BCM_PBMP_ITER(portConfig.port, port) {
    if (isPtpTcPcsSupported()) {
      disablePcsBasedTimestamping(port);
    }

    BCM_CHECK_ERROR(
        bcm_port_timesync_config_set(unit, port, 0 /* config_count */, NULL),
        "Error in disabling TC correction for port ",
        port);

    count++;
  }

  disableTimeInterface();
  XLOG(INFO) << "[PTP] Disabled timestamping on " << count << " ports";
}

} // namespace facebook::fboss
