/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/switch_asics/Jericho2Asic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/platforms/sai/SaiBcmDarwinPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmElbertPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmFujiPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmGalaxyPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmMinipackPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmMontblancPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge100PlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge400PlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge40PlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmYampPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiCloudRipperPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiElbert8DDPhyPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiFakePlatformPort.h"
#include "fboss/agent/platforms/sai/SaiLassenPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMeru400bfuPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMeru400biaPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMeru400biuPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMeru800bfaPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMeru800biaPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMorgan800ccPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiSandiaPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiWedge400CPlatformPort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include "fboss/agent/hw/sai/switch/SaiHandler.h"

DEFINE_string(
    dll_path,
    "/etc/packages/neteng-fboss-wedge_agent/current",
    "directory of HSDK warmboot DLL files");

DEFINE_string(
    firmware_path,
    "/etc/packages/neteng-fboss-wedge_agent/current",
    "Path to load the firmware");
namespace {

std::unordered_map<std::string, std::string> kSaiProfileValues;

const char* saiProfileGetValue(
    sai_switch_profile_id_t /*profile_id*/,
    const char* variable) {
  auto saiProfileValItr = kSaiProfileValues.find(variable);
  return saiProfileValItr != kSaiProfileValues.end()
      ? saiProfileValItr->second.c_str()
      : nullptr;
}

int saiProfileGetNextValue(
    sai_switch_profile_id_t /* profile_id */,
    const char** variable,
    const char** value) {
  static auto saiProfileValItr = kSaiProfileValues.begin();
  if (!value) {
    saiProfileValItr = kSaiProfileValues.begin();
    return 0;
  }
  if (saiProfileValItr == kSaiProfileValues.end()) {
    return -1;
  }
  *variable = saiProfileValItr->first.c_str();
  *value = saiProfileValItr->second.c_str();
  ++saiProfileValItr;
  return 0;
}

sai_service_method_table_t kSaiServiceMethodTable = {
    .profile_get_value = saiProfileGetValue,
    .profile_get_next_value = saiProfileGetNextValue,
};

using namespace facebook::fboss;
SaiSwitchTraits::Attributes::HwInfo getHwInfo(SaiPlatform* platform) {
  std::vector<int8_t> connectionHandle;
  // Use connection handle from switchInfo if available
  const auto agentConfig = platform->getConfig();
  const auto switchSettings = agentConfig->thrift.sw()->switchSettings();
  for (const auto& switchIdAndInfo : *switchSettings->switchIdToSwitchInfo()) {
    if (switchIdAndInfo.first == platform->getAsic()->getSwitchId()) {
      if (switchIdAndInfo.second.connectionHandle()) {
        auto connStr = *switchIdAndInfo.second.connectionHandle();
        std::copy(
            connStr.c_str(),
            connStr.c_str() + connStr.size() + 1,
            std::back_inserter(connectionHandle));
        return connectionHandle;
      }
    }
  }
  auto connStr = platform->getPlatformAttribute(
      cfg::PlatformAttributes::CONNECTION_HANDLE);
  if (connStr.has_value()) {
    std::copy(
        connStr->c_str(),
        connStr->c_str() + connStr->size() + 1,
        std::back_inserter(connectionHandle));
  }
  return connectionHandle;
}
} // namespace

namespace facebook::fboss {

SaiPlatform::SaiPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping,
    folly::MacAddress localMac)
    : Platform(std::move(productInfo), std::move(platformMapping), localMac) {
  const auto& portsByMasterPort =
      utility::getSubsidiaryPortIDs(getPlatformPorts());
  const auto& platPorts = getPlatformPorts();
  CHECK(portsByMasterPort.size() > 1);
  for (auto itPort : portsByMasterPort) {
    if (FLAGS_hide_fabric_ports) {
      if (*platPorts.find(static_cast<int32_t>(itPort.first))
               ->second.mapping()
               ->portType() != cfg::PortType::FABRIC_PORT) {
        masterLogicalPortIds_.push_back(itPort.first);
      }
    } else {
      masterLogicalPortIds_.push_back(itPort.first);
    }
  }
}

SaiPlatform::~SaiPlatform() {}

void SaiPlatform::updatePorts(const StateDelta& delta) {
  for (const auto& entry : delta.getPortsDelta()) {
    const auto newPort = entry.getNew();
    if (newPort) {
      getPort(newPort->getID())->portChanged(newPort, entry.getOld());
    }
  }
}

HwSwitch* SaiPlatform::getHwSwitch() const {
  return saiSwitch_.get();
}

void SaiPlatform::onHwInitialized(HwSwitchCallback* sw) {
  initLEDs();
  sw->registerStateObserver(this, "SaiPlatform");
}

void SaiPlatform::stateUpdated(const StateDelta& delta) {
  updatePorts(delta);
}

void SaiPlatform::onInitialConfigApplied(HwSwitchCallback* /* sw */) {}

void SaiPlatform::stop() {}

std::unique_ptr<ThriftHandler> SaiPlatform::createHandler(SwSwitch* sw) {
  return std::make_unique<SaiHandler>(sw, saiSwitch_.get());
}

std::shared_ptr<apache::thrift::AsyncProcessorFactory>
SaiPlatform::createHandler() {
  return std::make_shared<SaiHandler2>();
}

TransceiverIdxThrift SaiPlatform::getPortMapping(
    PortID portId,
    cfg::PortSpeed speed) const {
  return getPort(portId)->getTransceiverMapping(speed);
}

std::string SaiPlatform::getHwConfigDumpFile() {
  return getVolatileStateDir() + "/" + FLAGS_hw_config_file;
}

void SaiPlatform::generateHwConfigFile() {
  auto hwConfig = getHwConfig();
  if (!folly::writeFile(hwConfig, getHwConfigDumpFile().c_str())) {
    throw FbossError(errno, "failed to generate hw config file. write failed");
  }
}

void SaiPlatform::initSaiProfileValues() {
  kSaiProfileValues.insert(
      std::make_pair(SAI_KEY_INIT_CONFIG_FILE, getHwConfigDumpFile()));
  kSaiProfileValues.insert(std::make_pair(
      SAI_KEY_WARM_BOOT_READ_FILE, getWarmBootHelper()->warmBootDataPath()));
  kSaiProfileValues.insert(std::make_pair(
      SAI_KEY_WARM_BOOT_WRITE_FILE, getWarmBootHelper()->warmBootDataPath()));
  kSaiProfileValues.insert(std::make_pair(
      SAI_KEY_BOOT_TYPE, getWarmBootHelper()->canWarmBoot() ? "1" : "0"));
  auto vendorProfileValues = getSaiProfileVendorExtensionValues();
  kSaiProfileValues.insert(
      vendorProfileValues.begin(), vendorProfileValues.end());
  kSaiProfileValues.insert(std::make_pair(
      "SAI_SDK_LOG_CONFIG_FILE", "/root/res/config/sai_sdk_log_config.json"));
}

void SaiPlatform::initImpl(uint32_t hwFeaturesDesired) {
  initSaiProfileValues();
  SaiApiTable::getInstance()->queryApis(
      getServiceMethodTable(), getSupportedApiList());
  saiSwitch_ = std::make_unique<SaiSwitch>(this, hwFeaturesDesired);
  generateHwConfigFile();
}

void SaiPlatform::initPorts() {
  auto platformMode = getType();
  for (auto& port : getPlatformPorts()) {
    std::unique_ptr<SaiPlatformPort> saiPort;
    PortID portId(port.first);
    if (platformMode == PlatformType::PLATFORM_WEDGE400C ||
        platformMode == PlatformType::PLATFORM_WEDGE400C_VOQ ||
        platformMode == PlatformType::PLATFORM_WEDGE400C_FABRIC) {
      saiPort = std::make_unique<SaiWedge400CPlatformPort>(portId, this);
    } else if (
        platformMode == PlatformType::PLATFORM_CLOUDRIPPER ||
        platformMode == PlatformType::PLATFORM_CLOUDRIPPER_VOQ ||
        platformMode == PlatformType::PLATFORM_CLOUDRIPPER_FABRIC) {
      saiPort = std::make_unique<SaiCloudRipperPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_WEDGE) {
      saiPort = std::make_unique<SaiBcmWedge40PlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_WEDGE100) {
      saiPort = std::make_unique<SaiBcmWedge100PlatformPort>(portId, this);
    } else if (
        platformMode == PlatformType::PLATFORM_GALAXY_LC ||
        platformMode == PlatformType::PLATFORM_GALAXY_FC) {
      saiPort = std::make_unique<SaiBcmGalaxyPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_WEDGE400) {
      saiPort = std::make_unique<SaiBcmWedge400PlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_DARWIN) {
      saiPort = std::make_unique<SaiBcmDarwinPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MINIPACK) {
      saiPort = std::make_unique<SaiBcmMinipackPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MORGAN800CC) {
      saiPort = std::make_unique<SaiMorgan800ccPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_YAMP) {
      saiPort = std::make_unique<SaiBcmYampPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_FUJI) {
      saiPort = std::make_unique<SaiBcmFujiPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_ELBERT) {
      if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_ELBERT_8DD) {
        saiPort = std::make_unique<SaiElbert8DDPhyPlatformPort>(portId, this);
      } else if (
          getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK4) {
        saiPort = std::make_unique<SaiBcmElbertPlatformPort>(portId, this);
      }
    } else if (platformMode == PlatformType::PLATFORM_LASSEN) {
      saiPort = std::make_unique<SaiLassenPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_SANDIA) {
      saiPort = std::make_unique<SaiSandiaPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MERU400BIU) {
      saiPort = std::make_unique<SaiMeru400biuPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MERU800BIA) {
      saiPort = std::make_unique<SaiMeru800biaPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MERU400BIA) {
      saiPort = std::make_unique<SaiMeru400biaPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MERU400BFU) {
      saiPort = std::make_unique<SaiMeru400bfuPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MERU800BFA) {
      saiPort = std::make_unique<SaiMeru800bfaPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MONTBLANC) {
      saiPort = std::make_unique<SaiBcmMontblancPlatformPort>(portId, this);
    } else {
      saiPort = std::make_unique<SaiFakePlatformPort>(portId, this);
    }
    portMapping_.insert(std::make_pair(portId, std::move(saiPort)));
  }
}

SaiPlatformPort* SaiPlatform::getPort(PortID id) const {
  auto saiPortIter = portMapping_.find(id);
  if (saiPortIter == portMapping_.end()) {
    throw FbossError("failed to find port: ", id);
  }
  return saiPortIter->second.get();
}

PlatformPort* SaiPlatform::getPlatformPort(PortID port) const {
  return getPort(port);
}

sai_service_method_table_t* SaiPlatform::getServiceMethodTable() const {
  return &kSaiServiceMethodTable;
}

HwSwitchWarmBootHelper* SaiPlatform::getWarmBootHelper() {
  if (!wbHelper_) {
    wbHelper_ = std::make_unique<HwSwitchWarmBootHelper>(
        0, getWarmBootDir(), "sai_adaptor_state_");
  }
  return wbHelper_.get();
}

PortID SaiPlatform::findPortID(
    cfg::PortSpeed speed,
    std::vector<uint32_t> lanes,
    PortSaiId portSaiId) const {
  for (const auto& portMapping : portMapping_) {
    const auto& platformPort = portMapping.second;
    if (!platformPort->getProfileIDBySpeedIf(speed) ||
        platformPort->getHwPortLanes(speed) != lanes) {
      continue;
    }
    return platformPort->getPortID();
  }
  throw FbossError(
      "platform port not found ", (PortID)portSaiId, " speed: ", (int)speed);
}

std::vector<SaiPlatformPort*> SaiPlatform::getPortsWithTransceiverID(
    TransceiverID id) const {
  std::vector<SaiPlatformPort*> ports;
  for (const auto& port : portMapping_) {
    if (auto tcvrID = port.second->getTransceiverID()) {
      if (tcvrID == id &&
          port.second->getCurrentProfile() !=
              cfg::PortProfileID::PROFILE_DEFAULT) {
        ports.push_back(port.second.get());
      }
    }
  }
  return ports;
}

SaiSwitchTraits::CreateAttributes SaiPlatform::getSwitchAttributes(
    bool mandatoryOnly,
    cfg::SwitchType swType,
    std::optional<int64_t> swId) {
  SaiSwitchTraits::Attributes::InitSwitch initSwitch(true);

  std::optional<SaiSwitchTraits::Attributes::HwInfo> hwInfo = getHwInfo(this);
  std::optional<SaiSwitchTraits::Attributes::SrcMac> srcMac;
  std::optional<SaiSwitchTraits::Attributes::MacAgingTime> macAgingTime;
  if (!mandatoryOnly) {
    srcMac = getLocalMac();
    macAgingTime = getDefaultMacAgingTime();
  }

  std::optional<SaiSwitchTraits::Attributes::AclFieldList> aclFieldList =
      getAclFieldList();

  std::optional<SaiSwitchTraits::Attributes::UseEcnThresholds> useEcnThresholds{
      std::nullopt};
  if (getAsic()->isSupported(HwAsic::Feature::SAI_ECN_WRED)) {
    useEcnThresholds = true;
  }
  std::optional<SaiSwitchTraits::Attributes::SwitchType> switchType;
  std::optional<SaiSwitchTraits::Attributes::SwitchId> switchId;
  std::optional<SaiSwitchTraits::Attributes::MaxSystemCores> cores;
  std::optional<SaiSwitchTraits::Attributes::MaxCores> maxCores;
  std::optional<SaiSwitchTraits::Attributes::SysPortConfigList> sysPortConfigs;
  if (swType == cfg::SwitchType::VOQ || swType == cfg::SwitchType::FABRIC) {
    switchType = swType == cfg::SwitchType::VOQ ? SAI_SWITCH_TYPE_VOQ
                                                : SAI_SWITCH_TYPE_FABRIC;
    switchId = swId;
    if (swType == cfg::SwitchType::VOQ) {
      auto agentCfg = config();
      CHECK(agentCfg) << " agent config must be set ";
      uint32_t systemCores = 0;
      auto localMac = getLocalMac();
      const EbroAsic ebro(cfg::SwitchType::VOQ, 0, std::nullopt, localMac);
      const Jericho2Asic j2(cfg::SwitchType::VOQ, 0, std::nullopt, localMac);
      const Jericho3Asic j3(cfg::SwitchType::VOQ, 0, std::nullopt, localMac);
      for (const auto& [id, dsfNode] : *agentCfg->thrift.sw()->dsfNodes()) {
        if (dsfNode.type() != cfg::DsfNodeType::INTERFACE_NODE) {
          continue;
        }
        switch (*dsfNode.asicType()) {
          case cfg::AsicType::ASIC_TYPE_JERICHO2:
            systemCores += j2.getNumCores();
            // for directly connected interface nodes we don't expect
            // asic type to change across dsf nodes
            maxCores = j2.getNumCores();
            break;
          case cfg::AsicType::ASIC_TYPE_JERICHO3:
            systemCores += j3.getNumCores();
            // for directly connected interface nodes we don't expect
            // asic type to change across dsf nodes
            maxCores = j3.getNumCores();
            break;
          case cfg::AsicType::ASIC_TYPE_EBRO:
            systemCores += ebro.getNumCores();
            break;
          default:
            throw FbossError("Unexpected asic type: ", *dsfNode.asicType());
        }
      }
      cores = systemCores;
      sysPortConfigs = SaiSwitchTraits::Attributes::SysPortConfigList{
          getInternalSystemPortConfig()};
    }
  }
  std::optional<SaiSwitchTraits::Attributes::DllPath> dllPath;
#if defined(SAI_VERSION_8_2_0_0_ODP) ||     \
    defined(SAI_VERSION_8_2_0_0_SIM_ODP) || \
    defined(SAI_VERSION_9_2_0_0_ODP) || defined(SAI_VERSION_9_0_EA_SIM_ODP)
  auto platformMode = getType();
  if (platformMode == PlatformType::PLATFORM_FUJI ||
      platformMode == PlatformType::PLATFORM_ELBERT) {
    std::vector<int8_t> dllPathCharArray;
    std::copy(
        FLAGS_dll_path.c_str(),
        FLAGS_dll_path.c_str() + FLAGS_dll_path.size() + 1,
        std::back_inserter(dllPathCharArray));
    dllPath = dllPathCharArray;
  }
#endif

  std::optional<SaiSwitchTraits::Attributes::FirmwarePathName> firmwarePathName{
      std::nullopt};
  if (getAsic()->isSupported(HwAsic::Feature::SAI_FIRMWARE_PATH)) {
    std::vector<int8_t> firmwarePathNameArray;
    std::copy(
        FLAGS_firmware_path.c_str(),
        FLAGS_firmware_path.c_str() + FLAGS_firmware_path.size() + 1,
        std::back_inserter(firmwarePathNameArray));
    firmwarePathName = firmwarePathNameArray;
  }
  return {
    initSwitch,
        hwInfo, // hardware info
        srcMac, // source mac
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
        macAgingTime,
        std::nullopt, // ingress acl
        aclFieldList,
        std::nullopt, // tam object list
        useEcnThresholds,
        std::nullopt, // counter refresh interval
        firmwarePathName, // Firmware path name
        std::nullopt, // Firmware load method
        std::nullopt, // Firmware load type
        std::nullopt, // Hardware access bus
        std::nullopt, // Platform context
        std::nullopt, // Switch profile id
        switchId, // Switch id
        cores,
        sysPortConfigs, // System port config list
        switchType,
        std::nullopt, // Read function
        std::nullopt, // Write function
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
        std::nullopt, // Max ECMP member count
        std::nullopt, // ECMP member count
#endif
        dllPath,
        std::nullopt, // Restart Issu
        std::nullopt, // Switch Isolate
        std::nullopt, // Credit Watchdog
        maxCores, // Max cores
  };
}

std::vector<sai_system_port_config_t> SaiPlatform::getInternalSystemPortConfig()
    const {
  throw FbossError(
      "System port config must be provided by derived class platform");
}

uint32_t SaiPlatform::getDefaultMacAgingTime() const {
  static auto constexpr kL2AgeTimerSeconds = 300;
  return kL2AgeTimerSeconds;
}

const std::set<sai_api_t>& SaiPlatform::getDefaultSwitchAsicSupportedApis()
    const {
  // For now, most of the apis are supported for switch asic.
  // Therefore we can just reuse SaiApiTable::getFullApiList()
  // But in case in the future we have some special phy api which won't be
  // supported in the switch asic, we can also exclude those apis ad-hoc.
  static auto apis = SaiApiTable::getInstance()->getFullApiList();
  // Macsec is not currently supported in the broadcom sai sdk
  apis.erase(facebook::fboss::MacsecApi::ApiType);
  return apis;
}
const std::set<sai_api_t>& SaiPlatform::getDefaultPhyAsicSupportedApis() const {
  static const std::set<sai_api_t> kSupportedPhyApiList = {
      facebook::fboss::AclApi::ApiType,
      facebook::fboss::PortApi::ApiType,
      facebook::fboss::SwitchApi::ApiType,
  };
  return kSupportedPhyApiList;
}

const std::set<sai_api_t>& SaiPlatform::getSupportedApiList() const {
  return getDefaultSwitchAsicSupportedApis();
}

} // namespace facebook::fboss
