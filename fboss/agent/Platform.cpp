/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"

#include "fboss/agent/Utils.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"

#include <string>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossError.h"

DEFINE_string(
    crash_switch_state_file,
    "crash_switch_state",
    "File for dumping SwitchState state on crash");

DEFINE_string(
    crash_hw_state_file,
    "crash_hw_state",
    "File for dumping HW state on crash");

namespace facebook::fboss {

Platform::Platform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping)
    : productInfo_(std::move(productInfo)),
      platformMapping_(std::move(platformMapping)) {}
Platform::~Platform() {}

std::string Platform::getCrashHwStateFile() const {
  return getCrashInfoDir() + "/" + FLAGS_crash_hw_state_file;
}

std::string Platform::getCrashSwitchStateFile() const {
  return getCrashInfoDir() + "/" + FLAGS_crash_switch_state_file;
}

const AgentConfig* Platform::config() {
  if (!config_) {
    return reloadConfig();
  }
  return config_.get();
}

const AgentConfig* Platform::reloadConfig() {
  config_ = AgentConfig::fromDefaultFile();
  return config_.get();
}

void Platform::setConfig(std::unique_ptr<AgentConfig> config) {
  config_ = std::move(config);
}

const std::map<int32_t, cfg::PlatformPortEntry>& Platform::getPlatformPorts()
    const {
  return platformMapping_->getPlatformPorts();
}

const std::optional<phy::PortProfileConfig> Platform::getPortProfileConfig(
    cfg::PortProfileID profileID,
    std::optional<ExtendedSpecComplianceCode> transceiverSpecComplianceCode)
    const {
  return getPlatformMapping()->getPortProfileConfig(
      profileID, transceiverSpecComplianceCode);
}

const std::map<cfg::PortProfileID, phy::PortProfileConfig>&
Platform::getSupportedProfiles() {
  return platformMapping_->getSupportedProfiles();
}

const std::optional<phy::DataPlanePhyChip> Platform::getDataPlanePhyChip(
    std::string chipName) const {
  const auto& chips = getDataPlanePhyChips();
  if (auto chip = chips.find(chipName); chip != chips.end()) {
    return chip->second;
  } else {
    return std::nullopt;
  }
}

const std::map<std::string, phy::DataPlanePhyChip>&
Platform::getDataPlanePhyChips() const {
  return platformMapping_->getChips();
}

cfg::PortSpeed Platform::getPortMaxSpeed(PortID portID) const {
  return platformMapping_->getPortMaxSpeed(portID);
}

void Platform::init(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired) {
  // take ownership of the config if passed in
  config_ = std::move(config);
  std::ignore = getLocalMac();
  initImpl(hwFeaturesDesired);
  // We should always initPorts() here instead of leaving the hw/ to call
  initPorts();
}

folly::MacAddress Platform::getLocalMac() const {
  static std::optional<folly::MacAddress> kLocalMac;
  if (!kLocalMac.has_value()) {
    kLocalMac = getLocalMacAddress();
  }
  return kLocalMac.value();
}

void Platform::getProductInfo(ProductInfo& info) {
  CHECK(productInfo_);
  productInfo_->getInfo(info);
}

PlatformMode Platform::getMode() const {
  CHECK(productInfo_);
  return productInfo_->getMode();
}

phy::FecMode Platform::getPhyFecMode(cfg::PortProfileID profileID) const {
  auto profile = getPortProfileConfig(profileID);
  if (!profile) {
    throw FbossError("platform does not support profile : ", profileID);
  }
  return *profile->iphy_ref()->fec_ref();
}

void Platform::setPort2OverrideTransceiverInfo(
    const std::map<PortID, TransceiverInfo>& port2TransceiverInfo) {
  port2OverrideTransceiverInfo_.emplace(port2TransceiverInfo);
}

std::optional<std::map<PortID, TransceiverInfo>>
Platform::getPort2OverrideTransceiverInfo() const {
  return port2OverrideTransceiverInfo_;
}

std::optional<TransceiverInfo> Platform::getOverrideTransceiverInfo(
    PortID port) const {
  if (!port2OverrideTransceiverInfo_) {
    return std::nullopt;
  }
  // only for test environments this will be set, to avoid querying QSFP in
  // HwTest
  auto iter = port2OverrideTransceiverInfo_->find(port);
  if (iter == port2OverrideTransceiverInfo_->end()) {
    return std::nullopt;
  }
  return iter->second;
}

int Platform::getLaneCount(cfg::PortProfileID profile) const {
  switch (profile) {
    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_CL74_COPPER:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL:
      return 1;

    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_OPTICAL:
      return 2;

    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528:
    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N:
    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL:
    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER:
    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_NOFEC_COPPER:
      return 4;

    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N:
    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL:
      return 8;

    case cfg::PortProfileID::PROFILE_DEFAULT:
      break;
  }
  return 1;
}
} // namespace facebook::fboss
