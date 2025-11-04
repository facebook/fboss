/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/phy/SaiPhyRetimer.h"

#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"

#include <fmt/format.h>

namespace facebook::fboss::phy {

bool SaiPhyRetimer::isSupported(Feature feature) const {
  switch (feature) {
    case Feature::PRBS:
    case Feature::PRBS_STATS:
    case Feature::LOOPBACK:
    case Feature::PORT_STATS:
    case Feature::PORT_INFO:
    case Feature::MACSEC:
      return false;
    default:
      return true;
  }
}

void SaiPhyRetimer::dumpImpl() const {
  // An utility to make sure the firmware is loaded successfully
  auto switchId = getSwitchId();
  auto fwStatus = SaiApiTable::getInstance()->switchApi().getAttribute(
      switchId, SaiSwitchTraits::Attributes::FirmwareStatus{});
  const auto& fw = fwVersionImpl();
  if (auto versionStr = fw.versionStr()) {
    XLOG(INFO) << "Firmware status for " << name_ << ", switchId=" << switchId
               << ", version=" << *versionStr
               << ", status=" << (fwStatus ? "running" : "not_running");
  } else {
    throw FbossError(
        "Unable to get firmware version for ", name_, ", switchId=", switchId);
  }
}

PhyFwVersion SaiPhyRetimer::fwVersionImpl() const {
  auto switchId = getSwitchId();
  phy::PhyFwVersion fw;
  fw.version() = SaiApiTable::getInstance()->switchApi().getAttribute(
      switchId, SaiSwitchTraits::Attributes::FirmwareMajorVersion{});
  fw.minorVersion() = SaiApiTable::getInstance()->switchApi().getAttribute(
      switchId, SaiSwitchTraits::Attributes::FirmwareMinorVersion{});
  fw.versionStr() = fmt::format("{}.{}", *fw.version(), *fw.minorVersion());
  if (auto versionStr = fw.versionStr()) {
    XLOG(DBG5) << "fwVersionImpl switchId=" << switchId
               << ", version=" << *versionStr;
  }
  return fw;
}

} // namespace facebook::fboss::phy
