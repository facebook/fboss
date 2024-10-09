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

#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/switch_asics/Jericho2Asic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/platforms/sai/SaiBcmDarwinPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmElbertPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmFujiPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmMinipackPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmMontblancPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge100PlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmWedge400PlatformPort.h"
#include "fboss/agent/platforms/sai/SaiBcmYampPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiCloudRipperPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiElbert8DDPhyPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiFakePlatformPort.h"
#include "fboss/agent/platforms/sai/SaiJanga800bicPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMeru400bfuPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMeru400biaPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMeru400biuPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMeru800bfaPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMeru800biaPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiMorgan800ccPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiTahan800bcPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiWedge400CPlatformPort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/lib/CommonFileUtils.h"
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

DEFINE_bool(
    enable_delay_drop_congestion_threshold,
    false,
    "Enable new delay drop congestion threshold in CGM");

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
  for (const auto& itPort : portsByMasterPort) {
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

SaiPlatform::~SaiPlatform() = default;

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
  /*
   * In multiswitch mode, the platform is part hw agent
   * binary and has no access to swswitch. Skip state
   * observer registration. The state updates will be
   * propagated to platform as part of oper delta sync
   * from sw agent to hw agent
   */
  if (sw) {
    sw->registerStateObserver(this, "SaiPlatform");
  }
}

void SaiPlatform::stateUpdated(const StateDelta& delta) {
  updatePorts(delta);
}

std::shared_ptr<apache::thrift::AsyncProcessorFactory>
SaiPlatform::createHandler() {
  return std::make_shared<SaiHandler>(saiSwitch_.get());
}

TransceiverIdxThrift SaiPlatform::getPortMapping(
    PortID portId,
    cfg::PortProfileID profileID) const {
  return getPort(portId)->getTransceiverMapping(profileID);
}

std::string SaiPlatform::getHwConfigDumpFile() {
  return getDirectoryUtil()->getVolatileStateDir() + "/" +
      FLAGS_hw_config_file + "_idx" + std::to_string(FLAGS_switchIndex);
}

std::string SaiPlatform::getHwConfigDumpFile_deprecated() {
  return getDirectoryUtil()->getVolatileStateDir() + "/" + FLAGS_hw_config_file;
}

void SaiPlatform::generateHwConfigFile() {
  auto hwConfig = getHwConfig();
  if (!folly::writeFile(hwConfig, getHwConfigDumpFile().c_str())) {
    throw FbossError(errno, "failed to generate hw config file. write failed");
  }
}

std::string SaiPlatform::getHwAsicConfig(
    const std::unordered_map<std::string, std::string>& overrides) {
  /*
   * This function is used to dump the HW config into a file based
   * on new asic config format. Newer platforms will begin using
   * this format.
   *
   * Asic Config V2 has 2 components:
   * 1. Common Config
   * 2. NPU Specific Config
   *
   * Common Config contains all the common config parameters that are
   * applicable across all NPUs.
   *
   * NPU Specific Config contains all the NPU specific config parameters.
   *
   * Based on the switch index, hw config is generated by reading the
   * corresponding NPU config and appending it to the common config.
   *
   * Example:
   * If we have 4 NPUs, then the hw config will be of the following format:
   * - Common Config
   * - NPU 0 Config
   * - NPU 1 Config
   * - NPU 2 Config
   * - NPU 3 Config
   *
   * There will be 4 hw_config files created one per NPU. The filename
   * will be <hw_config_file>_<switch_index>.
   *
   * If the chip config type is not ASIC_CONFIG_V2, then an exception will
   * be thrown to fallback to bcm or asic config.
   */
  auto chipConfigType = config()->thrift.platform()->chip()->getType();
  if (chipConfigType != facebook::fboss::cfg::ChipConfig::asicConfig) {
    throw FbossError("No asic config v2 found in agent config");
  }
  auto asicConfig = config()->thrift.platform()->chip()->get_asicConfig();
  auto& commonConfigs = asicConfig.common()->get_config();
  std::vector<std::string> nameValStrs;
  auto addNameValue = [&nameValStrs, &overrides](const auto& keyAndVal) {
    auto oitr = overrides.find(keyAndVal.first);
    nameValStrs.emplace_back(folly::to<std::string>(
        keyAndVal.first,
        '=',
        oitr == overrides.end() ? keyAndVal.second : oitr->second));
  };
  for (const auto& entry : commonConfigs) {
    addNameValue(entry);
  }
  /*
   * Single NPU platfroms will not have any npu entries. In such cases,
   * we can directly use the common config.
   *
   * For multi-NPU platforms, we need to read the NPU specific config
   * by looking up the switch index from the list of npu entries.
   */
  if (asicConfig.npuEntries()) {
    auto& npuEntries = asicConfig.npuEntries().value();
    CHECK_LT(FLAGS_switchIndex, npuEntries.size());
    const auto& npuEntry = npuEntries.find(FLAGS_switchIndex);
    if (npuEntry != npuEntries.end()) {
      for (const auto& entry : npuEntry->second.get_config()) {
        addNameValue(entry);
      }
    }
  }
  auto hwConfig = folly::join('\n', nameValStrs);
  return hwConfig;
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
  auto switchId =
      SwitchID(getAsic()->getSwitchId() ? *getAsic()->getSwitchId() : 0);
  XLOG(DBG4) << "Platform ports are initialized with switch ID: " << switchId;
  HwSwitchMatcher matcher(std::unordered_set<SwitchID>({switchId}));
  for (auto& port : getPlatformPorts()) {
    std::unique_ptr<SaiPlatformPort> saiPort;
    PortID portId(port.first);
    if (scopeResolver()->scope(portId) != matcher) {
      XLOG(DBG5) << "Skipping platform port " << portId
                 << " due to scope mismatch";
      continue;
    }
    if (platformMode == PlatformType::PLATFORM_WEDGE400C ||
        platformMode == PlatformType::PLATFORM_WEDGE400C_VOQ ||
        platformMode == PlatformType::PLATFORM_WEDGE400C_FABRIC) {
      saiPort = std::make_unique<SaiWedge400CPlatformPort>(portId, this);
    } else if (
        platformMode == PlatformType::PLATFORM_CLOUDRIPPER ||
        platformMode == PlatformType::PLATFORM_CLOUDRIPPER_VOQ ||
        platformMode == PlatformType::PLATFORM_CLOUDRIPPER_FABRIC) {
      saiPort = std::make_unique<SaiCloudRipperPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_WEDGE100) {
      saiPort = std::make_unique<SaiBcmWedge100PlatformPort>(portId, this);
    } else if (
        platformMode == PlatformType::PLATFORM_WEDGE400 ||
        platformMode == PlatformType::PLATFORM_WEDGE400_GRANDTETON) {
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
    } else if (platformMode == PlatformType::PLATFORM_MERU400BIU) {
      saiPort = std::make_unique<SaiMeru400biuPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MERU800BIA) {
      saiPort = std::make_unique<SaiMeru800biaPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MERU400BIA) {
      saiPort = std::make_unique<SaiMeru400biaPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MERU400BFU) {
      saiPort = std::make_unique<SaiMeru400bfuPlatformPort>(portId, this);
    } else if (
        platformMode == PlatformType::PLATFORM_MERU800BFA ||
        platformMode == PlatformType::PLATFORM_MERU800BFA_P1) {
      saiPort = std::make_unique<SaiMeru800bfaPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_MONTBLANC) {
      saiPort = std::make_unique<SaiBcmMontblancPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_JANGA800BIC) {
      saiPort = std::make_unique<SaiJanga800bicPlatformPort>(portId, this);
    } else if (platformMode == PlatformType::PLATFORM_TAHAN800BC) {
      saiPort = std::make_unique<SaiTahan800bcPlatformPort>(portId, this);
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
        getAsic()->getSwitchIndex(),
        getDirectoryUtil()->getWarmBootDir(),
        "sai_adaptor_state_");
  }
  return wbHelper_.get();
}

std::pair<PortID, std::vector<cfg::PortProfileID>>
SaiPlatform::findPortIDAndProfiles(
    cfg::PortSpeed speed,
    std::vector<uint32_t> lanes,
    PortSaiId portSaiId) const {
  std::vector<cfg::PortProfileID> matchingProfiles;
  for (const auto& portMapping : portMapping_) {
    const auto& platformPort = portMapping.second;
    auto profiles = platformPort->getAllProfileIDsForSpeed(speed);
    for (auto profileID : profiles) {
      if (platformPort->getHwPortLanes(profileID) == lanes) {
        matchingProfiles.push_back(profileID);
      }
    }
    if (matchingProfiles.size() > 0) {
      return std::make_pair(platformPort->getPortID(), matchingProfiles);
    }
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
  if (!mandatoryOnly && swType != cfg::SwitchType::FABRIC) {
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
  std::optional<SaiSwitchTraits::Attributes::MaxSystemCores> maxSystemCores;
  std::optional<SaiSwitchTraits::Attributes::MaxCores> maxCores;
  std::optional<SaiSwitchTraits::Attributes::SysPortConfigList> sysPortConfigs;
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  std::optional<SaiSwitchTraits::Attributes::CreditWd> creditWd;
  std::optional<SaiSwitchTraits::Attributes::CreditWdTimer> creditWdMs;
  if (getAsic()->isSupported(HwAsic::Feature::CREDIT_WATCHDOG)) {
    // Use SAI defaults
    creditWd = true;
    creditWdMs = 500;
  }
#endif
  if (swType == cfg::SwitchType::VOQ || swType == cfg::SwitchType::FABRIC) {
    switchType = swType == cfg::SwitchType::VOQ ? SAI_SWITCH_TYPE_VOQ
                                                : SAI_SWITCH_TYPE_FABRIC;
    switchId = swId;
    if (swType == cfg::SwitchType::VOQ) {
      auto agentCfg = config();
      CHECK(agentCfg) << " agent config must be set ";
      uint32_t maxCoreCount = 0;
      uint32_t maxSystemCoreCount = 0;
      auto localMac = getLocalMac();
      const EbroAsic ebro(
          cfg::SwitchType::VOQ, 0, 0, std::nullopt, localMac, std::nullopt);
      const Jericho2Asic j2(
          cfg::SwitchType::VOQ, 0, 0, std::nullopt, localMac, std::nullopt);
      const Jericho3Asic j3(
          cfg::SwitchType::VOQ, 0, 0, std::nullopt, localMac, std::nullopt);
      for (const auto& [id, dsfNode] : *agentCfg->thrift.sw()->dsfNodes()) {
        if (dsfNode.type() != cfg::DsfNodeType::INTERFACE_NODE) {
          continue;
        }
        switch (*dsfNode.asicType()) {
          case cfg::AsicType::ASIC_TYPE_JERICHO2:
            // for directly connected interface nodes we don't expect
            // asic type to change across dsf nodes
            maxCoreCount = std::max(j2.getNumCores(), maxCoreCount);
            maxSystemCoreCount =
                std::max(maxSystemCoreCount, uint32_t(id + j2.getNumCores()));
            break;
          case cfg::AsicType::ASIC_TYPE_JERICHO3:
            // for directly connected interface nodes we don't expect
            // asic type to change across dsf nodes
            maxCoreCount = std::max(j3.getNumCores(), maxCoreCount);
            maxSystemCoreCount =
                std::max(maxSystemCoreCount, uint32_t(id + j3.getNumCores()));
            break;
          case cfg::AsicType::ASIC_TYPE_EBRO:
            break;
          default:
            throw FbossError("Unexpected asic type: ", *dsfNode.asicType());
        }
      }

      maxCores = maxCoreCount;
      maxSystemCores = maxSystemCoreCount;
      sysPortConfigs = SaiSwitchTraits::Attributes::SysPortConfigList{
          getInternalSystemPortConfig()};
    }
  }
  std::optional<SaiSwitchTraits::Attributes::DllPath> dllPath;
#if defined(BRCM_SAI_SDK_XGS)
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

  std::optional<SaiSwitchTraits::Attributes::SwitchIsolate> switchIsolate{
      std::nullopt};
  if (getAsic()->isSupported(HwAsic::Feature::LINK_INACTIVE_BASED_ISOLATE)) {
    switchIsolate = true;
  }

  std::optional<int32_t> voqLatencyMinLocalNs;
  std::optional<int32_t> voqLatencyMaxLocalNs;
  std::optional<int32_t> voqLatencyMinLevel1Ns;
  std::optional<int32_t> voqLatencyMaxLevel1Ns;
  std::optional<int32_t> voqLatencyMinLevel2Ns;
  std::optional<int32_t> voqLatencyMaxLevel2Ns;
  // TODO: Look at making this part of config instead of hardcoding
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_11_0)
  voqLatencyMinLocalNs = 5000;
  voqLatencyMaxLocalNs = 4294967295;
  voqLatencyMinLevel1Ns = 500000;
  voqLatencyMaxLevel1Ns = 4294967295;
  voqLatencyMinLevel2Ns = 800000;
  voqLatencyMaxLevel2Ns = 4294967295;
#endif

  std::optional<SaiSwitchTraits::Attributes::RouteNoImplicitMetaData>
      routeNoImplicitMetaData{std::nullopt};
#if defined(BRCM_SAI_SDK_GTE_11_0)
  if (getAsic()->isSupported(HwAsic::Feature::ROUTE_METADATA) &&
      FLAGS_set_classid_for_my_subnet_and_ip_routes) {
    XLOG(DBG2) << "Disable configuring implicit route metadata by sai adapter";
    routeNoImplicitMetaData = true;
  }
#endif
  std::optional<SaiSwitchTraits::Attributes::DelayDropCongThreshold>
      delayDropCongThreshold{std::nullopt};
#if defined(TAJO_SDK_VERSION_1_42_8)
  if (getAsic()->isSupported(
          HwAsic::Feature::ENABLE_DELAY_DROP_CONGESTION_THRESHOLD) &&
      FLAGS_enable_delay_drop_congestion_threshold) {
    XLOG(DBG2) << "Enable new CGM delay drop congestion thresholds";
    delayDropCongThreshold = 1;
  }
#endif
  std::optional<
      SaiSwitchTraits::Attributes::FabricLinkLayerFlowControlThreshold>
      fabricLLFC;
#if defined(BRCM_SAI_SDK_DNX) && defined(BRCM_SAI_SDK_GTE_12_0)
  if (getAsic()->getSwitchType() == cfg::SwitchType::FABRIC &&
      getAsic()->getFabricNodeRole() == HwAsic::FabricNodeRole::DUAL_STAGE_L1) {
    CHECK(getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_RAMON3)
        << " LLFC threshold values for no R3 chips in DUAL_STAGE_L1 role needs to figured out";
    // Vendor suggested valie
    constexpr uint32_t kRamon3LlfcThreshold{800};
    fabricLLFC = std::vector<uint32_t>({kRamon3LlfcThreshold});
  }
#endif

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
      maxSystemCores,
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
      switchIsolate,
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
      creditWd, // Credit Watchdog
      creditWdMs, // Credit Watchdog Timer
#endif
      maxCores, // Max cores
      std::nullopt, // PFC DLR Packet Action
      routeNoImplicitMetaData, // route no implicit meta data
      std::nullopt, // route allow implicit meta data
      std::nullopt, // multi-stage local switch ids
      voqLatencyMinLocalNs, // Local VoQ latency bin min
      voqLatencyMaxLocalNs, // Local VoQ latency bin max
      voqLatencyMinLevel1Ns, // Level1 VoQ latency bin min
      voqLatencyMaxLevel1Ns, // Level1 VoQ latency bin max
      voqLatencyMinLevel2Ns, // Level2 VoQ latency bin min
      voqLatencyMaxLevel2Ns, // Level2 VoQ latency bin max
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      std::nullopt, // ARS profile
#endif
      std::nullopt, // ReachabilityGroupList
      delayDropCongThreshold, // Delay Drop Cong Threshold
      fabricLLFC,
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

void SaiPlatform::stateChanged(const StateDelta& delta) {
  updatePorts(delta);
}

} // namespace facebook::fboss
