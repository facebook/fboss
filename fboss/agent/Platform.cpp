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

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/Utils.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

#include <folly/logging/xlog.h>
#include <string>

DEFINE_string(
    crash_switch_state_file,
    "crash_switch_state",
    "File for dumping SwitchState state on crash");

DEFINE_string(
    crash_thrift_switch_state_file,
    "crash_thrift_switch_state",
    "File for dumping SwitchState thrift on crash");

DEFINE_string(
    crash_hw_state_file,
    "crash_hw_state",
    "File for dumping HW state on crash");

DEFINE_string(
    hw_config_file,
    "hw_config",
    "File for dumping HW config on startup");

DEFINE_string(
    volatile_state_dir,
    "/dev/shm/fboss",
    "Directory for storing volatile state");
DEFINE_string(
    persistent_state_dir,
    "/var/facebook/fboss",
    "Directory for storing persistent state");

DEFINE_string(
    volatile_state_dir_phy,
    "/dev/shm/fboss/qsfp_service/phy",
    "Directory for storing phy volatile state");
DEFINE_string(
    persistent_state_dir_phy,
    "/var/facebook/fboss/qsfp_service/phy",
    "Directory for storing phy persistent state");

DEFINE_bool(hide_fabric_ports, false, "Elide ports of type fabric");

namespace facebook::fboss {

Platform::Platform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping,
    folly::MacAddress localMac)
    : productInfo_(std::move(productInfo)),
      platformMapping_(std::move(platformMapping)),
      localMac_(localMac) {}
Platform::~Platform() {}

std::string Platform::getCrashHwStateFile() const {
  return getCrashInfoDir() + "/" + FLAGS_crash_hw_state_file;
}

std::string Platform::getCrashSwitchStateFile() const {
  return getCrashInfoDir() + "/" + FLAGS_crash_switch_state_file;
}

std::string Platform::getCrashThriftSwitchStateFile() const {
  return getCrashInfoDir() + "/" + FLAGS_crash_thrift_switch_state_file;
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
    PlatformPortProfileConfigMatcher profileMatcher) const {
  return getPlatformMapping()->getPortProfileConfig(profileMatcher);
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
  auto macStr = getPlatformAttribute(cfg::PlatformAttributes::MAC);
  const auto switchSettings = *config_->thrift.sw()->switchSettings();
  std::optional<int64_t> switchId;
  std::optional<cfg::Range64> systemPortRange;
  auto switchType{cfg::SwitchType::NPU};
  if (switchSettings.switchIdToSwitchInfo()->size()) {
    // TODO - Initialize with correct SwitchId for the HwSwitch
    // instead of using first switchId in the list
    switchId = switchSettings.switchIdToSwitchInfo()->begin()->first;
    switchType =
        *(switchSettings.switchIdToSwitchInfo()->begin()->second.switchType());
    auto asicType =
        *(switchSettings.switchIdToSwitchInfo()->begin()->second.asicType());
    if (switchType == cfg::SwitchType::VOQ) {
      const auto& dsfNodesConfig = *config_->thrift.sw()->dsfNodes();
      const auto& dsfNodeConfig = dsfNodesConfig.find(*switchId);
      if (dsfNodeConfig != dsfNodesConfig.end() &&
          dsfNodeConfig->second.systemPortRange().has_value()) {
        systemPortRange = *dsfNodeConfig->second.systemPortRange();
      }
    }
    // SwitchId not supported in fabric mode
    if (switchType == cfg::SwitchType::FABRIC &&
        (asicType == cfg::AsicType::ASIC_TYPE_EBRO ||
         asicType == cfg::AsicType::ASIC_TYPE_GARONNE)) {
      switchId = std::nullopt;
    }
    if (switchSettings.switchIdToSwitchInfo()->begin()->second.switchMac()) {
      macStr =
          *switchSettings.switchIdToSwitchInfo()->begin()->second.switchMac();
    }
  }
  // Override local mac from config if set
  if (macStr) {
    XLOG(DBG2) << " Setting platform mac to: " << macStr.value();
    localMac_ = folly::MacAddress(*macStr);
  }
  setupAsic(switchType, switchId, systemPortRange, localMac_);
  initImpl(hwFeaturesDesired);
  // We should always initPorts() here instead of leaving the hw/ to call
  initPorts();
}

void Platform::getProductInfo(ProductInfo& info) {
  CHECK(productInfo_);
  productInfo_->getInfo(info);
}

PlatformType Platform::getType() const {
  CHECK(productInfo_);
  return productInfo_->getType();
}

void Platform::setOverrideTransceiverInfo(
    const TransceiverInfo& overrideTransceiverInfo) {
  // Use the template to create TransceiverInfo map based on PlatformMapping
  std::unordered_map<TransceiverID, TransceiverInfo> overrideTcvrs;
  for (const auto& port : getPlatformPorts()) {
    auto portID = PortID(port.first);
    auto platformPort = getPlatformPort(portID);
    if (auto transceiverID = platformPort->getTransceiverID(); transceiverID &&
        overrideTcvrs.find(*transceiverID) == overrideTcvrs.end()) {
      // Use the overrideTransceiverInfo_ as template to copy a new
      // TransceiverInfo with the corresponding TransceiverID
      auto tcvrInfo = TransceiverInfo(overrideTransceiverInfo);
      tcvrInfo.port() = *transceiverID;
      overrideTcvrs.emplace(*transceiverID, tcvrInfo);
    }
  }
  XLOG(DBG2) << "Build override TransceiverInfo map, size="
             << overrideTcvrs.size();
  overrideTransceiverInfos_.emplace(overrideTcvrs);
}

std::optional<TransceiverInfo> Platform::getOverrideTransceiverInfo(
    PortID port) const {
  if (!overrideTransceiverInfos_) {
    return std::nullopt;
  }
  // only for test environments this will be set, to avoid querying QSFP in
  // HwTest
  if (auto tcvrID = getPlatformPort(port)->getTransceiverID()) {
    if (auto overrideTcvrInfo = overrideTransceiverInfos_->find(*tcvrID);
        overrideTcvrInfo != overrideTransceiverInfos_->end()) {
      return overrideTcvrInfo->second;
    }
  }
  return std::nullopt;
}

std::optional<std::unordered_map<TransceiverID, TransceiverInfo>>
Platform::getOverrideTransceiverInfos() const {
  return overrideTransceiverInfos_;
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
    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_COPPER_RACK_YV3_T1:
    case cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER:
    case cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL:
    case cfg::PortProfileID::PROFILE_106POINT25G_1_PAM4_RS544_COPPER:
    case cfg::PortProfileID::PROFILE_106POINT25G_1_PAM4_RS544_OPTICAL:
      return 1;

    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC:
    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_20G_2_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_50G_2_NRZ_NOFEC_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL:
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
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER_RACK_YV3_T1:
    case cfg::PortProfileID::PROFILE_400G_4_PAM4_RS544X2N_OPTICAL:
      return 4;

    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N:
    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL:
    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_COPPER:
    case cfg::PortProfileID::PROFILE_800G_8_PAM4_RS544X2N_OPTICAL:
      return 8;

    case cfg::PortProfileID::PROFILE_DEFAULT:
      break;
  }
  return 1;
}

uint32_t Platform::getMMUCellBytes() const {
  throw FbossError("MMU Cell bytes not defined for this platform");
}

std::optional<std::string> Platform::getPlatformAttribute(
    cfg::PlatformAttributes platformAttribute) const {
  const auto& platform = *config_->thrift.platform();

  if (auto platformSettings = platform.platformSettings()) {
    auto platformIter = platformSettings->find(platformAttribute);
    if (platformIter == platformSettings->end()) {
      return std::nullopt;
    }
    return platformIter->second;
  } else {
    return std::nullopt;
  }
}

} // namespace facebook::fboss
