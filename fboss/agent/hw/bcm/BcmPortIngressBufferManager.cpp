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
// defaults in mmu_lossless=1 mode
constexpr bcm_cosq_control_drop_limit_alpha_value_t kDefaultPgAlpha =
    bcmCosqControlDropLimitAlpha_8;
constexpr int kDefaultPortPgId = 0;
constexpr int kDefaultMinLimitBytes = 0;
constexpr int kDefaultHeadroomLimitBytes = 0;
constexpr int kdefaultResumeOffsetBytes = 0;
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
      " cosq ",
      cosq,
      " value ",
      value);
}

void BcmPortIngressBufferManager::readCosqTypeFromHw(
    const int cosq,
    const bcm_cosq_control_t type,
    int* value,
    const std::string& typeStr) {
  *value = 0;
  auto rv = bcm_cosq_control_get(unit_, gport_, cosq, type, value);
  bcmCheckError(
      rv, "failed to get ", typeStr, " for port ", portName_, " cosq ", cosq);
}

void BcmPortIngressBufferManager::programPg(
    const PortPgConfig* portPgCfg,
    const int cosq) {
  int sharedDynamicEnable = 1;
  const auto& scalingFactor = portPgCfg->getScalingFactor();
  XLOG(DBG2) << "Program port PG config for cosq: " << cosq
             << " on port: " << portName_;

  if (!scalingFactor) {
    sharedDynamicEnable = 0;
  }

  writeCosqTypeToHw(
      cosq,
      bcmCosqControlIngressPortPGSharedDynamicEnable,
      sharedDynamicEnable,
      "bcmCosqControlIngressPortPGSharedDynamicEnable");

  if (sharedDynamicEnable) {
    auto alpha = utility::cfgAlphaToBcmAlpha(*scalingFactor);
    writeCosqTypeToHw(
        cosq,
        bcmCosqControlDropLimitAlpha,
        alpha,
        "bcmCosqControlDropLimitAlpha");
  }

  int pgMinLimitBytes = portPgCfg->getMinLimitBytes();
  writeCosqTypeToHw(
      cosq,
      bcmCosqControlIngressPortPGMinLimitBytes,
      pgMinLimitBytes,
      "bcmCosqControlIngressPortPGMinLimitBytes");

  auto hdrmBytes = portPgCfg->getHeadroomLimitBytes();
  int headroomBytes = hdrmBytes ? *hdrmBytes : 0;
  writeCosqTypeToHw(
      cosq,
      bcmCosqControlIngressPortPGHeadroomLimitBytes,
      headroomBytes,
      "bcmCosqControlIngressPortPGHeadroomLimitBytes");

  auto resumeBytes = portPgCfg->getResumeOffsetBytes();
  if (resumeBytes) {
    writeCosqTypeToHw(
        cosq,
        bcmCosqControlIngressPortPGResetOffsetBytes,
        *resumeBytes,
        "bcmCosqControlIngressPortPGResetOffsetBytes");
  }
}

void BcmPortIngressBufferManager::resetPgToDefault(int pgId) {
  const auto& portPg = getDefaultPgSettings();
  programPg(&portPg, pgId);
}

void BcmPortIngressBufferManager::resetPgsToDefault() {
  XLOG(DBG2) << "Reset all programmed PGs to default for port " << portName_;
  auto pgIdList = getPgIdListInHw();
  for (const auto& pgId : pgIdList) {
    resetPgToDefault(pgId);
  }
  pgIdList.clear();
  setPgIdListInHw(pgIdList);
}

void BcmPortIngressBufferManager::reprogramPgs(
    const std::shared_ptr<Port> port) {
  PgIdSet newPgList = {};
  const auto portPgCfgs = port->getPortPgConfigs();
  const auto pgIdList = getPgIdListInHw();

  if (portPgCfgs) {
    for (const auto& portPgCfg : *portPgCfgs) {
      programPg(portPgCfg.get(), portPgCfg->getID());
      newPgList.insert(portPgCfg->getID());
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
  XLOG(DBG2) << "New PG list programmed for port " << portName_;
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
    resetPgsToDefault();
    return;
  }

  // simply reprogram based on new config
  // case 3, 4
  reprogramPgs(port);
}

const PortPgConfig& getTH3DefaultPgSettings() {
  static const PortPgConfig portPgConfig{PortPgFields{
      .id = kDefaultPortPgId,
      .scalingFactor = utility::bcmAlphaToCfgAlpha(kDefaultPgAlpha),
      .name = std::nullopt,
      .minLimitBytes = kDefaultMinLimitBytes,
      .headroomLimitBytes = kDefaultHeadroomLimitBytes,
      .resumeOffsetBytes = kdefaultResumeOffsetBytes,
      .bufferPoolName = "",
  }};
  return portPgConfig;
}

const PortPgConfig& BcmPortIngressBufferManager::getDefaultChipPgSettings(
    utility::BcmChip chip) {
  switch (chip) {
    case utility::BcmChip::TOMAHAWK3:
      return getTH3DefaultPgSettings();
    default:
      // currently ony supported for TH3
      throw FbossError("Unsupported platform for PG settings: ", chip);
  }
}

const PortPgConfig& BcmPortIngressBufferManager::getDefaultPgSettings() {
  return hw_->getPlatform()->getDefaultPortPgSettings();
}

void BcmPortIngressBufferManager::getPgParamsHw(
    const int pgId,
    const std::shared_ptr<PortPgConfig>& pg) {
  getIngressAlpha(pgId, pg);
  getPgMinLimitBytes(pgId, pg);
  getPgResumeOffsetBytes(pgId, pg);
  getPgHeadroomLimitBytes(pgId, pg);
}

PortPgConfigs BcmPortIngressBufferManager::getCurrentPgSettingsHw() {
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

PortPgConfigs BcmPortIngressBufferManager::getCurrentProgrammedPgSettingsHw() {
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

void BcmPortIngressBufferManager::getPgHeadroomLimitBytes(
    bcm_cos_queue_t cosQ,
    std::shared_ptr<PortPgConfig> pgConfig) {
  int headroomBytes = 0;
  readCosqTypeFromHw(
      cosQ,
      bcmCosqControlIngressPortPGHeadroomLimitBytes,
      &headroomBytes,
      "bcmCosqControlIngressPortPGHeadroomLimitBytes");
  pgConfig->setHeadroomLimitBytes(headroomBytes);
}

void BcmPortIngressBufferManager::getIngressAlpha(
    bcm_cos_queue_t cosQ,
    std::shared_ptr<PortPgConfig> pgConfig) {
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
    pgConfig->setScalingFactor(scalingFactor);
  }
}

void BcmPortIngressBufferManager::getPgMinLimitBytes(
    bcm_cos_queue_t cosQ,
    std::shared_ptr<PortPgConfig> pgConfig) {
  int minBytes = 0;
  readCosqTypeFromHw(
      cosQ,
      bcmCosqControlIngressPortPGMinLimitBytes,
      &minBytes,
      "bcmCosqControlIngressPortPGMinLimitBytes");
  pgConfig->setMinLimitBytes(minBytes);
}

void BcmPortIngressBufferManager::getPgResumeOffsetBytes(
    bcm_cos_queue_t cosQ,
    std::shared_ptr<PortPgConfig> pgConfig) {
  int resumeBytes = 0;
  readCosqTypeFromHw(
      cosQ,
      bcmCosqControlIngressPortPGResetOffsetBytes,
      &resumeBytes,
      "bcmCosqControlIngressPortPGResetOffsetBytes");
  pgConfig->setResumeOffsetBytes(resumeBytes);
}

PgIdSet BcmPortIngressBufferManager::getPgIdListInHw() {
  std::lock_guard<std::mutex> g(pgIdListLock_);
  return std::set<int>(pgIdListInHw_.begin(), pgIdListInHw_.end());
}

void BcmPortIngressBufferManager::setPgIdListInHw(PgIdSet& newPgIdList) {
  std::lock_guard<std::mutex> g(pgIdListLock_);
  pgIdListInHw_ = std::move(newPgIdList);
}

} // namespace facebook::fboss
