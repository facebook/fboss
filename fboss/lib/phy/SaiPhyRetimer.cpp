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

namespace {
constexpr uint32_t kPmdDeviceAddr = 1;
/*
 * This is a static function for reading a Phy register. This function will be
 * passed to the SAI layer. The SAI driver will use this function to read a
 * phy register. The SAI will call the function with platform context. We are
 * passing the platform context as the SaiPhyRetimer object pointer during
 * create switch. When SAI calls this function with platform context then this
 * function will use that context to get correct SaiPhyRetimer object and then
 * call that object's XphyIO's readRegister function
 */
sai_status_t saiPhyRetimerSwitchRegisterRead(
    uint64_t platform_context,
    uint32_t device_addr,
    uint32_t start_reg_addr,
    uint32_t number_of_registers,
    uint32_t* reg_val) {
  if (device_addr != kPmdDeviceAddr) {
    XLOG(ERR)
        << "saiPhyRetimerSwitchRegisterRead failed, bad device address passed";
    return SAI_STATUS_INVALID_PARAMETER;
  }

  SaiPhyRetimer* retimerObj =
      reinterpret_cast<SaiPhyRetimer*>(platform_context);
  CHECK(retimerObj);

  // Finally call that object's XphyIO read register function
  for (uint32_t i = 0; i < number_of_registers; i++) {
    uint16_t value = retimerObj->getXphyIO()->readRegister(
        retimerObj->getPhyAddr(),
        SaiPhyRetimer::getDeviceAddress(),
        static_cast<uint16_t>(start_reg_addr + i));
    reg_val[i] = value;
  }

  return SAI_STATUS_SUCCESS;
}

/*
 * This is a static function for writing a Phy register. This function will be
 * passed to the SAI layer. The SAI driver will use this function to write a
 * phy register. The SAI will call the function with platform context. We are
 * passing the platform context as the SaiPhyRetimer object pointer during
 * create switch. When SAI calls this function with platform context then this
 * function will use that context to get correct SaiPhyRetimer object and then
 * call that object's XphyIO's writeRegister function
 */
sai_status_t saiPhyRetimerSwitchRegisterWrite(
    uint64_t platform_context,
    uint32_t device_addr,
    uint32_t start_reg_addr,
    uint32_t number_of_registers,
    const uint32_t* reg_val) {
  if (device_addr != kPmdDeviceAddr) {
    XLOG(ERR)
        << "saiPhyRetimerSwitchRegisterWrite failed, bad device address passed";
    return SAI_STATUS_INVALID_PARAMETER;
  }

  // Find the SaiPhyRetimer object from platform context passed by SAI
  SaiPhyRetimer* retimerObj =
      reinterpret_cast<SaiPhyRetimer*>(platform_context);
  CHECK(retimerObj);

  // Finally call that object's XphyIO write register function
  for (uint32_t i = 0; i < number_of_registers; i++) {
    retimerObj->getXphyIO()->writeRegister(
        retimerObj->getPhyAddr(),
        SaiPhyRetimer::getDeviceAddress(),
        static_cast<uint16_t>(start_reg_addr + i),
        static_cast<uint16_t>(reg_val[i]));
  }

  return SAI_STATUS_SUCCESS;
}
} // namespace

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

void* SaiPhyRetimer::getRegisterReadFuncPtr() {
  return reinterpret_cast<void*>(saiPhyRetimerSwitchRegisterRead);
}

void* SaiPhyRetimer::getRegisterWriteFuncPtr() {
  return reinterpret_cast<void*>(saiPhyRetimerSwitchRegisterWrite);
}

} // namespace facebook::fboss::phy
