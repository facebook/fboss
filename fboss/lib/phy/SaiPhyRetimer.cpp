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

#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"

#include <fmt/format.h>

namespace facebook::fboss::phy {

namespace {
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
    uint32_t /*device_addr*/,
    uint32_t start_reg_addr,
    uint32_t number_of_registers,
    uint32_t* reg_val) {
  SaiPhyRetimer* retimerObj =
      reinterpret_cast<SaiPhyRetimer*>(platform_context);
  CHECK(retimerObj);

  XLOG(DBG5) << __func__ << ":"
             << " phyAddr=" << retimerObj->getPhyAddr()
             << " deviceAddr=" << SaiPhyRetimer::getDeviceAddress()
             << " start_reg_addr=" << start_reg_addr
             << " number_of_registers=" << number_of_registers;

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
    uint32_t /*device_addr*/,
    uint32_t start_reg_addr,
    uint32_t number_of_registers,
    const uint32_t* reg_val) {
  // Find the SaiPhyRetimer object from platform context passed by SAI
  SaiPhyRetimer* retimerObj =
      reinterpret_cast<SaiPhyRetimer*>(platform_context);
  CHECK(retimerObj);

  XLOG(DBG5) << __func__ << ":"
             << " phyAddr=" << retimerObj->getPhyAddr()
             << " deviceAddr=" << SaiPhyRetimer::getDeviceAddress()
             << " start_reg_addr=" << start_reg_addr
             << " number_of_registers=" << number_of_registers;
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
  XLOG(DBG5) << __func__ << ": Returning read function pointer";
  return reinterpret_cast<void*>(saiPhyRetimerSwitchRegisterRead);
}

void* SaiPhyRetimer::getRegisterWriteFuncPtr() {
  XLOG(DBG5) << __func__ << ": Returning write function pointer";
  return reinterpret_cast<void*>(saiPhyRetimerSwitchRegisterWrite);
}

SaiSwitchTraits::CreateAttributes SaiPhyRetimer::getSwitchAttributes() {
  XLOG(DBG5) << __func__ << ": Starting for xphyID=" << xphyID_;

  SaiSwitchTraits::Attributes::InitSwitch initSwitch(true);

  // Set HwInfo as XPHY ID
  // Encoding as 4-byte little-endian integer
  std::vector<int8_t> hwInfoVec(4);
  uint32_t phyId = static_cast<uint32_t>(xphyID_);
  hwInfoVec[0] = phyId & 0xFF;
  hwInfoVec[1] = (phyId >> 8) & 0xFF;
  hwInfoVec[2] = (phyId >> 16) & 0xFF;
  hwInfoVec[3] = (phyId >> 24) & 0xFF;
  XLOG(DBG2) << __func__ << ": Encoded phyId=" << phyId << " into hwInfoVec";
  std::optional<SaiSwitchTraits::Attributes::HwInfo> hwInfo(hwInfoVec);

  // Pass the static register read function
  std::optional<SaiSwitchTraits::Attributes::RegisterReadFn> regReadFn(
      getRegisterReadFuncPtr());

  // Pass the static register write function
  std::optional<SaiSwitchTraits::Attributes::RegisterWriteFn> regWriteFn(
      getRegisterWriteFuncPtr());

  // We are assuming this firmware is part of the SDK and dont need an external
  // fw
  std::optional<SaiSwitchTraits::Attributes::FirmwarePathName> fwPathName =
      std::nullopt;

  std::optional<SaiSwitchTraits::Attributes::FirmwareLoadMethod> fwLoadMethod(
      SAI_SWITCH_FIRMWARE_LOAD_METHOD_INTERNAL);

  std::optional<SaiSwitchTraits::Attributes::FirmwareLoadType> fwLoadType(
      SAI_SWITCH_FIRMWARE_LOAD_TYPE_AUTO);

  // Phy is accessible on MDIO
  std::optional<SaiSwitchTraits::Attributes::HardwareAccessBus> hwAccessBus(
      SAI_SWITCH_HARDWARE_ACCESS_BUS_MDIO);

  // Pass this object pointer as platform context to driver. The driver will
  // use this context while calling read/write register function and there we
  // will use this context to identify this object again
  std::optional<SaiSwitchTraits::Attributes::PlatformContext> context(
      reinterpret_cast<sai_uint64_t>(this));

  std::optional<SaiSwitchTraits::Attributes::SwitchType> switchType(
      SAI_SWITCH_TYPE_PHY);

  // Set profile ID to XPHY ID so each retimer gets unique profile id
  uint32_t profileIdValue = static_cast<uint32_t>(xphyID_);
  XLOG(DBG5) << __func__ << ": Setting profileId=" << profileIdValue;
  std::optional<SaiSwitchTraits::Attributes::SwitchProfileId> profileId(
      profileIdValue);

  // Mandatory attributes for Sai
  std::optional<SaiSwitchTraits::Attributes::SwitchId> switchId(0);
  std::optional<SaiSwitchTraits::Attributes::MaxSystemCores> maxSysCores(0);
  std::vector<sai_system_port_config_t> sysPortConfigVec;
  std::optional<SaiSwitchTraits::Attributes::SysPortConfigList>
      sysPortConfigList(sysPortConfigVec);

  return {
      initSwitch,
      hwInfo, // hardware info
      std::nullopt, // srcMac,
      std::nullopt, // shell
      std::nullopt, // ecmp hash v4
      std::nullopt, // ecmp hash v6
      std::nullopt, // lag hash v4
      std::nullopt, // lag hash v6
      std::nullopt, // ecmp hash seed
      std::nullopt, // lag hash seed
      std::nullopt, // ecmp hash algo
      std::nullopt, // lag hash algo
      std::nullopt, // restart warm
      std::nullopt, // qos dscp to tc map
      std::nullopt, // qos tc to queue map
      std::nullopt, // qos exp to tc map
      std::nullopt, // qos tc to exp map
      std::nullopt, // macAgingTime
      std::nullopt, // ingress acl
      std::nullopt, // egress acl
      std::nullopt, // aclFieldList
      std::nullopt, // tam object list
      std::nullopt, // use Ecn Thresholds
      std::nullopt, // counter refresh interval
      std::nullopt, // Firmware core to use
      fwPathName, // Firmware path name
      std::nullopt, // Firmware log file
      fwLoadMethod, // Firmware load method
      fwLoadType, // Firmware load type
      hwAccessBus, // Hardware access bus
      context, // Platform context
      profileId, // Switch profile id (set to XPHY ID)
      switchId, // Switch id
      maxSysCores, // Max system cores
      sysPortConfigList, // System port config list
      switchType, // Switch type
      regReadFn, // Register read function
      regWriteFn, // Register write function
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
      std::nullopt, // Max ECMP member count
      std::nullopt, // ECMP member count
#endif
      std::nullopt, // dllPath
      std::nullopt, // restartIssu
      std::nullopt, // Switch Isolate
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      std::nullopt, // Credit Watchdog
      std::nullopt, // Credit Watchdog Timer
#endif
      std::nullopt, // Max Cores
      std::nullopt, // PFC DLR Packet Action
      std::nullopt, // route no implicit meta data
      std::nullopt, // route allow implicit meta data
      std::nullopt, // multi-stage local switch ids
      std::nullopt, // Local VoQ latency bin min
      std::nullopt, // Local VoQ latency bin max
      std::nullopt, // Level1 VoQ latency bin min
      std::nullopt, // Level1 VoQ latency bin max
      std::nullopt, // Level2 VoQ latency bin min
      std::nullopt, // Level2 VoQ latency bin max
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      std::nullopt, // ArsProfile
#endif
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
      std::nullopt, // Port PTP mode
#endif
      std::nullopt, // Reachability Group List
      std::nullopt, // Delay Drop Cong Threshold
      std::nullopt, // Fabric link level control threshold
      std::nullopt, // SRAM free percent XOFF threshold
      std::nullopt, // SRAM free percent XON threshold
      std::nullopt, // No acls for traps
      std::nullopt, // Max system port ids
      std::nullopt, // Max local system port ids
      std::nullopt, // Max system ports
      std::nullopt, // Max voqs
      std::nullopt, // Fabric CLLFC TX credit threshold
      std::nullopt, // VOQ DRAM bound threshold
      std::nullopt, // ConditionalEntropyRehashPeriodUS
      std::nullopt, // Shel Source IP
      std::nullopt, // Shel Destination IP
      std::nullopt, // Shel Source MAC
      std::nullopt, // Shel Periodic Interval
      std::nullopt, // MaxSwitchId
      std::nullopt, // Sflow aggr number of samples
      std::nullopt, // SDK Register dump log path
      std::nullopt, // Firmware Object list
      std::nullopt, // tc rate limit list
      std::nullopt, // PFC watchdog timer granularity
      std::nullopt, // disable sll and hll timeout
      std::nullopt, // credit request profile scheduler mode
      std::nullopt, // module id to credit request profile param list
#if defined(BRCM_SAI_SDK_XGS_AND_DNX)
      std::nullopt, // range list of local scope system port ids
#endif
      std::nullopt, // enable PFC monitoring for the switch
      std::nullopt, // enable cable propagation delay measurement
  };
}

} // namespace facebook::fboss::phy
