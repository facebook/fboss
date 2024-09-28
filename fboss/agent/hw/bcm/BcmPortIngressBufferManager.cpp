/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmPortIngressBufferManager.h"
#include "fboss/agent/hw/bcm/BcmCosQueueFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/Port.h"

#include <folly/logging/xlog.h>

extern "C" {
#include <bcm/cosq.h>
#include <bcm/types.h>
}

namespace {
// defaults in mmu_lossless=0x2 mode
// determined by dumping registers from HW
constexpr bcm_cosq_control_drop_limit_alpha_value_t kDefaultPgAlpha =
    bcmCosqControlDropLimitAlpha_8;
constexpr int kDefaultPortPgId = 0;
constexpr int kDefaultMinLimitBytes = 0;
constexpr int kDefaultHeadroomLimitBytes = 0;
constexpr int kdefaultResumeOffsetBytes = 0;
constexpr int kDefaultSharedBytesTh3 = 111490 * 254;
constexpr int kDefaultHeadroomBytesTh3 = 18528 * 254;
// TODO(daiweix): just keep TH4 BcmPortIngressBufferManagerTest happy for now,
// need to update these default settings on TH4 later CS00012246702
constexpr int kDefaultSharedBytesTh4 = 111490 * 254;
constexpr int kDefaultHeadroomBytesTh4 = 18528 * 254;
// arbit
const std::string kDefaultBufferPoolName = "default";
constexpr int kDefaultPgId = 0;
} // unnamed namespace

namespace facebook::fboss {

BcmPortIngressBufferManager::BcmPortIngressBufferManager(
    BcmSwitch* hw,
    const std::string& portName,
    bcm_gport_t portGport)
    : hw_(hw), portName_(portName), gport_(portGport), unit_(hw_->getUnit()) {}

void BcmPortIngressBufferManager::writeCosqTypeToHw(
    const int cosq,
    const bcm_cosq_control_t type,
    const int value,
    const std::string& typeStr) {
  auto rv = bcm_cosq_control_set(unit_, gport_, cosq, type, value);
  bcmCheckError(
      rv,
      "failed to set ",
      typeStr,
      " for port ",
      portName_,
      " pgId ",
      cosq,
      " value ",
      value);
  XLOG(DBG2) << "Program " << typeStr << " for port " << portName_ << ", pgId "
             << cosq << " value " << value;
}

void BcmPortIngressBufferManager::readCosqTypeFromHw(
    const int cosq,
    const bcm_cosq_control_t type,
    int* value,
    const std::string& typeStr) const {
  *value = 0;
  auto rv = bcm_cosq_control_get(unit_, gport_, cosq, type, value);
  bcmCheckError(
      rv, "failed to get ", typeStr, " for port ", portName_, " cosq ", cosq);
}

void BcmPortIngressBufferManager::writeCosqTypeToHwIfNeeded(
    const int cosq,
    const bcm_cosq_control_t type,
    const int value,
    const std::string& typeStr) {
  int hwValue = 0;
  readCosqTypeFromHw(cosq, type, &hwValue, typeStr);
  // only program if needed - rewrites during warm boot can cause problem
  if (hwValue != value) {
    writeCosqTypeToHw(cosq, type, value, typeStr);
  } else {
    XLOG(DBG2) << "Skip programming " << typeStr << "value : " << value;
  }
}

void BcmPortIngressBufferManager::programPfcOnPgIfNeeded(
    const int cosq,
    const bool pfcEnable) {
  int newPfcStatus = pfcEnable ? 1 : 0;
  int currPfcStatus = getProgrammedPfcStatusInPg(cosq);
  if (currPfcStatus != newPfcStatus) {
    programPfcOnPg(cosq, newPfcStatus);
  } else {
    XLOG(DBG2) << "Skip programming for programPfcOnPgIfNeeded";
  }
}

void BcmPortIngressBufferManager::programPfcOnPg(
    const int cosq,
    const int pfcEnable) {
  bcm_port_priority_group_config_t pgConfig;
  bcm_port_priority_group_config_t_init(&pgConfig);

  pgConfig.pfc_transmit_enable = pfcEnable;
  auto localPort = BCM_GPORT_MODPORT_PORT_GET(gport_);
  auto rv =
      bcm_port_priority_group_config_set(unit_, localPort, cosq, &pgConfig);
  bcmCheckError(
      rv,
      "failed to program bcm_port_priority_group_config_set for port ",
      portName_,
      " for pgId ",
      cosq,
      " pfc_transmit_enable ",
      pfcEnable);
  XLOG(DBG2) << "Program port " << portName_ << ", pg " << cosq
             << "with pfcEnable " << pfcEnable;
}

int BcmPortIngressBufferManager::getProgrammedPgLosslessMode(
    const int pgId) const {
  int arg = 0;
  auto localPort = BCM_GPORT_MODPORT_PORT_GET(gport_);
  auto rv = bcm_cosq_port_priority_group_property_get(
      unit_, localPort, pgId, bcmCosqPriorityGroupLossless, &arg);
  bcmCheckError(
      rv,
      "failed to program bcm_cosq_port_priority_group_property_set for port ",
      portName_,
      " pgId ",
      pgId);
  return arg;
}

int BcmPortIngressBufferManager::getProgrammedPfcStatusInPg(
    const int pgId) const {
  bcm_port_priority_group_config_t pgConfig;
  bcm_port_priority_group_config_t_init(&pgConfig);

  auto localPort = BCM_GPORT_MODPORT_PORT_GET(gport_);
  auto rv =
      bcm_port_priority_group_config_get(unit_, localPort, pgId, &pgConfig);
  bcmCheckError(
      rv,
      "failed to read bcm_port_priority_group_config_get for port ",
      portName_,
      " for pgId ",
      pgId);
  return pgConfig.pfc_transmit_enable;
}

void BcmPortIngressBufferManager::programPg(
    state::PortPgFields portPgCfg,
    const int cosq) {
  int sharedDynamicEnable = 1;
  cfg::MMUScalingFactor scalingFactor;
  if (portPgCfg.scalingFactor()) {
    scalingFactor =
        nameToEnum<cfg::MMUScalingFactor>(*portPgCfg.scalingFactor());
  } else {
    sharedDynamicEnable = 0;
  }

  writeCosqTypeToHwIfNeeded(
      cosq,
      bcmCosqControlIngressPortPGSharedDynamicEnable,
      sharedDynamicEnable,
      "bcmCosqControlIngressPortPGSharedDynamicEnable");

  if (sharedDynamicEnable) {
    auto alpha = utility::cfgAlphaToBcmAlpha(scalingFactor);
    writeCosqTypeToHwIfNeeded(
        cosq,
        bcmCosqControlDropLimitAlpha,
        alpha,
        "bcmCosqControlDropLimitAlpha");
  }

  int pgMinLimitBytes = *portPgCfg.minLimitBytes();
  writeCosqTypeToHwIfNeeded(
      cosq,
      bcmCosqControlIngressPortPGMinLimitBytes,
      pgMinLimitBytes,
      "bcmCosqControlIngressPortPGMinLimitBytes");

  auto hdrmBytes = portPgCfg.headroomLimitBytes();
  int headroomBytes = hdrmBytes ? *hdrmBytes : 0;
  writeCosqTypeToHwIfNeeded(
      cosq,
      bcmCosqControlIngressPortPGHeadroomLimitBytes,
      headroomBytes,
      "bcmCosqControlIngressPortPGHeadroomLimitBytes");

  auto resumeBytes = portPgCfg.resumeOffsetBytes();
  if (resumeBytes) {
    writeCosqTypeToHwIfNeeded(
        cosq,
        bcmCosqControlIngressPortPGResetOffsetBytes,
        *resumeBytes,
        "bcmCosqControlIngressPortPGResetOffsetBytes");
  }
}

void BcmPortIngressBufferManager::resetPgToDefault(int pgId) {
  const auto& portPg = getDefaultPgSettings();
  // THRIFT_COPY
  programPg(portPg.getFields()->toThrift(), pgId);
  // disable pfc on default pgs
  programPfcOnPgIfNeeded(pgId, false);
}

void BcmPortIngressBufferManager::resetIngressPoolsToDefault() {
  XLOG(DBG2) << "Reset ingress service pools to default for port " << portName_;
  const auto& bufferPoolCfg = getDefaultIngressPoolSettings();

  // we use one common buffer pool across all ports/PGs in our implementation
  // SDK API forces us to use port, PG
  // To prevent multiple sdk calls for all PGs just reset for kDefaultPgId only,
  // as all PGs refer to the same buffer pool only
  writeCosqTypeToHwIfNeeded(
      kDefaultPgId,
      bcmCosqControlIngressPoolLimitBytes,
      bufferPoolCfg.getSharedBytes(),
      "bcmCosqControlIngressPoolLimitBytes");
  writeCosqTypeToHwIfNeeded(
      kDefaultPgId,
      bcmCosqControlIngressHeadroomPoolLimitBytes,
      *bufferPoolCfg.getHeadroomBytes(),
      "bcmCosqControlIngressHeadroomPoolLimitBytes");
}

void BcmPortIngressBufferManager::resetPgsToDefault(
    const std::shared_ptr<Port> port) {
  XLOG(DBG2) << "Reset all programmed PGs to default for port " << portName_;
  auto pgIdList = getPgIdListInHw();
  for (const auto& pgId : pgIdList) {
    resetPgToDefault(pgId);
  }
  pgIdList.clear();
  programLosslessMode(port);
  setPgIdListInHw(pgIdList);
  bufferPoolName_ = kDefaultBufferPoolName;
}

void BcmPortIngressBufferManager::programPgLosslessMode(int pgId, int value) {
  auto localPort = BCM_GPORT_MODPORT_PORT_GET(gport_);
  auto rv = bcm_cosq_port_priority_group_property_set(
      unit_, localPort, pgId, bcmCosqPriorityGroupLossless, value);
  bcmCheckError(
      rv,
      "failed to program bcm_cosq_port_priority_group_property_set for port ",
      portName_,
      " pgId ",
      pgId,
      " value: ",
      value);
  XLOG(DBG2) << "Set lossless mode  " << value << "for pgId " << pgId
             << " port " << portName_;
}

void BcmPortIngressBufferManager::programPgLosslessModeIfNeeded(
    int pgId,
    int newPgLosslessMode) {
  int currPgLosslessMode = getProgrammedPgLosslessMode(pgId);
  if (currPgLosslessMode != newPgLosslessMode) {
    programPgLosslessMode(pgId, newPgLosslessMode);
  } else {
    XLOG(DBG2) << "Skip programming for pgId: " << pgId;
  }
}

void BcmPortIngressBufferManager::programLosslessMode(
    const std::shared_ptr<Port> port) {
  std::unordered_set<int> losslessPgList;
  // theoretically its possible that each pg configuration defined is NOT
  // lossless look for headroom limit set to determine its lossless
  if (const auto& pgConfigs = port->getPortPgConfigs()) {
    for (const auto& pgConfig : *pgConfigs) {
      if (pgConfig->cref<switch_state_tags::headroomLimitBytes>()) {
        losslessPgList.insert(pgConfig->cref<switch_state_tags::id>()->cref());
      }
    }
  }

  for (int pgId = 0; pgId <= cfg::switch_config_constants::PORT_PG_VALUE_MAX();
       ++pgId) {
    int losslessMode = 0;
    auto iter = losslessPgList.find(pgId);
    if (iter != losslessPgList.end()) {
      losslessMode = 1;
    }
    programPgLosslessModeIfNeeded(pgId, losslessMode);
  }
}

void BcmPortIngressBufferManager::reprogramPgs(
    const std::shared_ptr<Port> port) {
  PgIdSet newPgList = {};
  const auto portPgCfgs = port->getPortPgConfigs();
  const auto pgIdList = getPgIdListInHw();
  bool isPfcEnabled = false;

  if (const auto& pfc = port->getPfc()) {
    isPfcEnabled = *pfc->tx() || *pfc->rx();
  }

  if (portPgCfgs) {
    for (const auto& portPgCfg : std::as_const(*portPgCfgs)) {
      programPg(
          portPgCfg->toThrift(),
          portPgCfg->cref<switch_state_tags::id>()->cref());
      // enable pfc on pg if so
      programPfcOnPgIfNeeded(
          portPgCfg->cref<switch_state_tags::id>()->cref(), isPfcEnabled);
      newPgList.insert(portPgCfg->cref<switch_state_tags::id>()->cref());
    }

    // find pgs in original list but not in new list
    // so we  know which ones to reset
    PgIdSet resetPgList;
    std::set_difference(
        pgIdList.begin(),
        pgIdList.end(),
        newPgList.begin(),
        newPgList.end(),
        std::inserter(resetPgList, resetPgList.end()));

    for (const auto pg : resetPgList) {
      XLOG(DBG2) << "Reset PG " << pg << " to default for port " << portName_;
      resetPgToDefault(pg);
    }
  }
  // update to latest PG list
  setPgIdListInHw(newPgList);
  programLosslessMode(port);
  XLOG(DBG2) << "New PG list programmed for port " << portName_;
}

void BcmPortIngressBufferManager::reprogramIngressPools(
    const std::shared_ptr<Port> port) {
  const auto& portPgCfgs = port->getPortPgConfigs();
  if (portPgCfgs->size()) {
    // All PGs should have the same buffer pool associated!
    bufferPoolName_ = (*std::as_const(*portPgCfgs).begin())
                          ->cref<switch_state_tags::bufferPoolName>()
                          ->toThrift();
  }
  for (const auto& portPgCfg : std::as_const(*portPgCfgs)) {
    auto portPgId = portPgCfg->cref<switch_state_tags::id>()->cref();
    if (auto bufferPoolCfg =
            portPgCfg->cref<switch_state_tags::bufferPoolConfig>()) {
      auto currentGlobalHeadroomBytes = getIngressPoolHeadroomBytes(portPgId);
      auto currentGlobalSharedBytes = getIngressSharedBytes(portPgId);
      auto newGlobalSharedBytes =
          bufferPoolCfg->cref<common_if_tags::sharedBytes>()->cref();
      auto newGlobalHeadroomBytes =
          bufferPoolCfg->cref<common_if_tags::headroomBytes>()->cref();

      // When we program shared pool, SDK runs a check on hdrm + shared buffer
      // and it shouldn't exceed the MAX. If we program shared buffer first
      // and its higher than default (common case),  hdrm + shared buffer > MAX
      // and SDK fails! Vice versa  can also happen where hdrm is higher if we
      // program hdrm first. One way to solve this is to program whichever one
      // is lower, so that SDK check for hdrm + shared < MAX always hold true
      if ((currentGlobalHeadroomBytes == newGlobalHeadroomBytes) &&
          (currentGlobalSharedBytes == newGlobalSharedBytes)) {
        // nothing to do if its the same
        XLOG(DBG2) << "No change in the global buffers for port: " << portName_;
        continue;
      }

      if (newGlobalHeadroomBytes < currentGlobalHeadroomBytes) {
        // new global hdrm is lower than current, lets program this one first
        setIngressPoolHeadroomBytes(portPgId, newGlobalHeadroomBytes);
        setIngressSharedBytes(portPgId, newGlobalSharedBytes);
      } else {
        // (1) new shared ingress pool is lower than current
        // (2) new shared ingress pool  > current
        // (3) new global hdroom pool > current
        setIngressSharedBytes(portPgId, newGlobalSharedBytes);
        setIngressPoolHeadroomBytes(portPgId, newGlobalHeadroomBytes);
      }
    }
  }
}

//  there are 4 possible cases
//  case 1: No prev cfg, no new cfg
//  case 2: Prev cfg, no new cfg
//  case 3: No prev cfg, new cfg
//  case 4: Prev cfg, new cfg
void BcmPortIngressBufferManager::programIngressBuffers(
    const std::shared_ptr<Port>& port) {
  const auto pgIdList = getPgIdListInHw();
  const auto& portPgCfgs = port->getPortPgConfigs();
  if (!portPgCfgs && (pgIdList.size() == 0)) {
    // there is nothing to program or unprogram
    // case 1
    return;
  }

  if (!portPgCfgs) {
    // unprogram the existing pgs
    // case 2
    resetPgsToDefault(port);
    // NOTE:
    // all ports map to 2 separate ingress pools (1 per ITM)
    // now if we reset to default for some port, it effectively happens
    // for all other ports in that ITM as well. Ideally ref count
    // all ports on ITM basis (complex) go to default when ref_count = 0
    // and hence invoke this call.
    // But practically speaking, we shouldn't have this case ...as ALL ports
    // should either point to the given buffer pool or ALL should point to
    // default
    resetIngressPoolsToDefault();
    return;
  }

  // simply reprogram based on new config
  // case 3, 4
  reprogramPgs(port);
  reprogramIngressPools(port);
}

const PortPgConfig& getDefaultPriorityGroupSettings() {
  static const PortPgConfig portPgConfig{PortPgConfig::makeThrift(
      kDefaultPortPgId, // id
      utility::bcmAlphaToCfgAlpha(kDefaultPgAlpha), // scaling factor
      std::nullopt, // name
      kDefaultMinLimitBytes, // minLimitBytes
      kDefaultHeadroomLimitBytes, // headroomLimitBytes
      kdefaultResumeOffsetBytes, // resumeOffsetBytes
      "" // bufferPoolName
      )};
  return portPgConfig;
}

const BufferPoolCfg& getTH3DefaultIngressPoolSettings() {
  static const BufferPoolCfg bufferPoolCfg(
      kDefaultBufferPoolName, kDefaultSharedBytesTh3, kDefaultHeadroomBytesTh3);
  return bufferPoolCfg;
}

const BufferPoolCfg& getTH4DefaultIngressPoolSettings() {
  static const BufferPoolCfg bufferPoolCfg(
      kDefaultBufferPoolName, kDefaultSharedBytesTh4, kDefaultHeadroomBytesTh4);
  return bufferPoolCfg;
}

// static
const PortPgConfig& BcmPortIngressBufferManager::getDefaultChipPgSettings(
    utility::BcmChip chip) {
  switch (chip) {
    case utility::BcmChip::TOMAHAWK3:
    case utility::BcmChip::TOMAHAWK4:
      // TODO(daiweix): update default settings for TH4
      return getDefaultPriorityGroupSettings();
    default:
      // currently ony supported for TH3
      throw FbossError("Unsupported platform for PG settings: ", chip);
  }
}

// static
const BufferPoolCfg&
BcmPortIngressBufferManager::getDefaultChipIngressPoolSettings(
    utility::BcmChip chip) {
  switch (chip) {
    case utility::BcmChip::TOMAHAWK3:
      return getTH3DefaultIngressPoolSettings();
    case utility::BcmChip::TOMAHAWK4:
      return getTH4DefaultIngressPoolSettings();
    default:
      // currently ony supported for TH3
      throw FbossError(
          "Unsupported platform for Ingress Pool settings: ", chip);
  }
}

const PortPgConfig& BcmPortIngressBufferManager::getDefaultPgSettings() const {
  return hw_->getPlatform()->getDefaultPortPgSettings();
}

const BufferPoolCfg&
BcmPortIngressBufferManager::getDefaultIngressPoolSettings() const {
  return hw_->getPlatform()->getDefaultPortIngressPoolSettings();
}

void BcmPortIngressBufferManager::getPgParamsHw(
    const int pgId,
    const std::shared_ptr<PortPgConfig>& pg) const {
  if (const auto alpha = getIngressAlpha(pgId)) {
    pg->setScalingFactor(alpha.value());
  }
  pg->setMinLimitBytes(getPgMinLimitBytes(pgId));
  pg->setResumeOffsetBytes(getPgResumeOffsetBytes(pgId));
  pg->setHeadroomLimitBytes(getPgHeadroomLimitBytes(pgId));
}

BufferPoolCfgPtr BcmPortIngressBufferManager::getCurrentIngressPoolSettings()
    const {
  auto cfg = std::make_shared<BufferPoolCfg>(bufferPoolName_);
  // pick the settings for pgid = 0, since its global pool
  // all others will have the same values
  cfg->setHeadroomBytes(getIngressPoolHeadroomBytes(kDefaultPgId));
  cfg->setSharedBytes(getIngressSharedBytes(kDefaultPgId));
  return cfg;
}

PortPgConfigs BcmPortIngressBufferManager::getCurrentPgSettingsHw() const {
  PortPgConfigs pgs = {};
  // walk all pgs in HW and derive the programmed values
  for (auto pgId = 0; pgId <= cfg::switch_config_constants::PORT_PG_VALUE_MAX();
       pgId++) {
    auto pg = std::make_shared<PortPgConfig>(static_cast<uint8_t>(pgId));
    getPgParamsHw(pgId, pg);
    pgs.emplace_back(pg);
  }
  return pgs;
}

PortPgConfigs BcmPortIngressBufferManager::getCurrentProgrammedPgSettingsHw()
    const {
  PortPgConfigs pgs = {};

  // walk all programmed list of the pgIds in the order {0 -> 7}
  // Retrive copy of pgIdsListInHw_
  // But if pgIdsListInHw_ is not programmed, we return back empty
  auto pgIdList = getPgIdListInHw();
  for (const auto pgId : pgIdList) {
    auto pg = std::make_shared<PortPgConfig>(static_cast<uint8_t>(pgId));
    getPgParamsHw(pgId, pg);
    pgs.emplace_back(pg);
  }
  return pgs;
}

void BcmPortIngressBufferManager::setIngressPoolHeadroomBytes(
    const bcm_cos_queue_t cosQ,
    const int headroomBytes) {
  writeCosqTypeToHw(
      cosQ,
      bcmCosqControlIngressHeadroomPoolLimitBytes,
      headroomBytes,
      "bcmCosqControlIngressHeadroomPoolLimitBytes");
}

void BcmPortIngressBufferManager::setIngressSharedBytes(
    const bcm_cos_queue_t cosQ,
    const int sharedBytes) {
  writeCosqTypeToHw(
      cosQ,
      bcmCosqControlIngressPoolLimitBytes,
      sharedBytes,
      "bcmCosqControlIngressPoolLimitBytes");
}

int BcmPortIngressBufferManager::getIngressPoolHeadroomBytes(
    bcm_cos_queue_t cosQ) const {
  int headroomBytes = 0;
  readCosqTypeFromHw(
      cosQ,
      bcmCosqControlIngressHeadroomPoolLimitBytes,
      &headroomBytes,
      "bcmCosqControlIngressHeadroomPoolLimitBytes");
  return headroomBytes;
}

int BcmPortIngressBufferManager::getIngressSharedBytes(
    bcm_cos_queue_t cosQ) const {
  int sharedBytes = 0;
  readCosqTypeFromHw(
      cosQ,
      bcmCosqControlIngressPoolLimitBytes,
      &sharedBytes,
      "bcmCosqControlIngressPoolLimitBytes");
  return sharedBytes;
}

int BcmPortIngressBufferManager::getPgHeadroomLimitBytes(
    bcm_cos_queue_t cosQ) const {
  int headroomBytes = 0;
  readCosqTypeFromHw(
      cosQ,
      bcmCosqControlIngressPortPGHeadroomLimitBytes,
      &headroomBytes,
      "bcmCosqControlIngressPortPGHeadroomLimitBytes");
  return headroomBytes;
}

std::optional<cfg::MMUScalingFactor>
BcmPortIngressBufferManager::getIngressAlpha(bcm_cos_queue_t cosQ) const {
  int sharedDynamicEnable = 0;
  readCosqTypeFromHw(
      cosQ,
      bcmCosqControlIngressPortPGSharedDynamicEnable,
      &sharedDynamicEnable,
      "bcmCosqControlIngressPortPGSharedDynamicEnable");
  if (sharedDynamicEnable) {
    int bcmAlpha = 0;
    readCosqTypeFromHw(
        cosQ,
        bcmCosqControlDropLimitAlpha,
        &bcmAlpha,
        "bcmCosqControlDropLimitAlpha");
    auto scalingFactor = utility::bcmAlphaToCfgAlpha(
        static_cast<bcm_cosq_control_drop_limit_alpha_value_e>(bcmAlpha));
    return scalingFactor;
  }
  return std::nullopt;
}

int BcmPortIngressBufferManager::getPgMinLimitBytes(const int pgId) const {
  int minBytes = 0;
  bcm_cos_queue_t cosQ = pgId;
  readCosqTypeFromHw(
      cosQ,
      bcmCosqControlIngressPortPGMinLimitBytes,
      &minBytes,
      "bcmCosqControlIngressPortPGMinLimitBytes");
  return minBytes;
}

int BcmPortIngressBufferManager::getPgResumeOffsetBytes(
    bcm_cos_queue_t cosQ) const {
  int resumeBytes = 0;
  readCosqTypeFromHw(
      cosQ,
      bcmCosqControlIngressPortPGResetOffsetBytes,
      &resumeBytes,
      "bcmCosqControlIngressPortPGResetOffsetBytes");
  return resumeBytes;
}

PgIdSet BcmPortIngressBufferManager::getPgIdListInHw() const {
  std::lock_guard<std::mutex> g(pgIdListLock_);
  return std::set<int>(pgIdListInHw_.begin(), pgIdListInHw_.end());
}

void BcmPortIngressBufferManager::setPgIdListInHw(PgIdSet& newPgIdList) {
  std::lock_guard<std::mutex> g(pgIdListLock_);
  pgIdListInHw_ = std::move(newPgIdList);
}

} // namespace facebook::fboss
