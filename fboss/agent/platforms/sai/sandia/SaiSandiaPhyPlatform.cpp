/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/sandia/SaiSandiaPhyPlatform.h"
#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/MarvelPhyAsic.h"
#include "fboss/agent/platforms/common/MultiPimPlatformMapping.h"
#include "fboss/agent/platforms/common/sandia/SandiaPlatformMapping.h"

namespace facebook::fboss {

const std::string& SaiSandiaPhyPlatform::getFirmwareDirectory() {
  static const std::string kFirmwareDir = "/lib/firmware/fboss/mvl/88x93161/";
  return kFirmwareDir;
}

namespace {
// SAI profile values for warm-boot and initial config
// The map key includes profile Id (representing XPHY switch) and the key
// string
std::map<std::string, std::string> kSaiProfileValues;

/*
 * saiProfileGetValue
 *
 * This function returns some key values to the SAI while doing
 * sai_api_initialize.
 *  SAI_KEY_BOOT_TYPE - Return warm(1) or cold(0) depending on warm-boot helper
 *    file presence logic
 *  SAI_KEY_INIT_CONFIG_FILE - Hw config dump file location
 *  SAI_KEY_WARM_BOOT_READ_FILE - Warm boot state read file (accessed on init)
 *  SAI_KEY_WARM_BOOT_WRITE_FILE - Warm boot state write file (accessed during
 *    graceful exit)
 */
const char* FOLLY_NULLABLE
saiProfileGetValue(sai_switch_profile_id_t profileId, const char* variable) {
  std::string profileMapKey =
      folly::sformat("XPHY{:d}_{:s}", profileId, variable);
  auto saiProfileValItr = kSaiProfileValues.find(profileMapKey);
  return saiProfileValItr != kSaiProfileValues.end()
      ? saiProfileValItr->second.c_str()
      : nullptr;
}

/*
 * saiProfileGetNextValue
 *
 * This function lets SAI pick up next value for a given key
 */
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
} // namespace

SaiSandiaPhyPlatform::SaiSandiaPhyPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    uint8_t pimId,
    int phyId)
    : SaiHwPlatform(
          std::move(productInfo),
          std::make_unique<SandiaPlatformMapping>(),
          localMac),
      pimId_(pimId),
      phyId_(phyId) {}

void SaiSandiaPhyPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    std::optional<cfg::Range64> systemPortRange) {
  asic_ =
      std::make_unique<MarvelPhyAsic>(switchType, switchId, systemPortRange);
}
SaiSandiaPhyPlatform::~SaiSandiaPhyPlatform() {}

std::string SaiSandiaPhyPlatform::getHwConfig() {
  throw FbossError("SaiSandiaPhyPlatform doesn't support getHwConfig()");
}
HwAsic* SaiSandiaPhyPlatform::getAsic() const {
  return asic_.get();
}

std::vector<PortID> SaiSandiaPhyPlatform::getAllPortsInGroup(
    PortID /* portID */) const {
  throw FbossError("SaiSandiaPhyPlatform doesn't support FlexPort");
}
std::vector<FlexPortMode> SaiSandiaPhyPlatform::getSupportedFlexPortModes()
    const {
  throw FbossError("SaiSandiaPhyPlatform doesn't support FlexPort");
}

std::optional<sai_port_interface_type_t> SaiSandiaPhyPlatform::getInterfaceType(
    TransmitterTechnology transmitterTech,
    cfg::PortSpeed speed) const {
  if (!getAsic()->isSupported(HwAsic::Feature::PORT_INTERFACE_TYPE)) {
    return std::nullopt;
  }

  static std::map<
      cfg::PortSpeed,
      std::map<TransmitterTechnology, sai_port_interface_type_t>>
      kSpeedAndMediaType2InterfaceType = {
          {cfg::PortSpeed::HUNDREDG,
           {{TransmitterTechnology::OPTICAL, SAI_PORT_INTERFACE_TYPE_SR4},
            {TransmitterTechnology::BACKPLANE, SAI_PORT_INTERFACE_TYPE_SR2},
            // What to default to
            {TransmitterTechnology::COPPER, SAI_PORT_INTERFACE_TYPE_SR4},
            {TransmitterTechnology::UNKNOWN, SAI_PORT_INTERFACE_TYPE_SR4}}},
          {cfg::PortSpeed::TWOHUNDREDG,
           {{TransmitterTechnology::OPTICAL, SAI_PORT_INTERFACE_TYPE_SR4},
            {TransmitterTechnology::BACKPLANE, SAI_PORT_INTERFACE_TYPE_SR2},
            // What to default to
            {TransmitterTechnology::COPPER, SAI_PORT_INTERFACE_TYPE_SR4},
            {TransmitterTechnology::UNKNOWN, SAI_PORT_INTERFACE_TYPE_SR4}}},
          {cfg::PortSpeed::FOURHUNDREDG,
           // TODO(rajank):
           // Replace 22 with SR8 once wedge_qsfp_util is build for
           // SDK >=1.10.0
           {{TransmitterTechnology::OPTICAL, sai_port_interface_type_t(22)},
            {TransmitterTechnology::BACKPLANE, SAI_PORT_INTERFACE_TYPE_SR4},
            // What to default to
            {TransmitterTechnology::COPPER, SAI_PORT_INTERFACE_TYPE_SR4},
            {TransmitterTechnology::UNKNOWN, SAI_PORT_INTERFACE_TYPE_SR4}}},
      };

  auto mediaType2InterfaceTypeIter =
      kSpeedAndMediaType2InterfaceType.find(speed);
  if (mediaType2InterfaceTypeIter == kSpeedAndMediaType2InterfaceType.end()) {
    throw FbossError(
        "unsupported speed for interface type retrieval : ", speed);
  }

  auto interfaceTypeIter =
      mediaType2InterfaceTypeIter->second.find(transmitterTech);
  if (interfaceTypeIter == mediaType2InterfaceTypeIter->second.end()) {
    throw FbossError(
        "unsupported media type for interface type retrieval : ",
        transmitterTech);
  }
  XLOG(DBG3) << "getInterfaceType for speed " << static_cast<int>(speed)
             << " transmitterTech " << static_cast<int>(transmitterTech)
             << " interfaceType = " << interfaceTypeIter->second;

  return interfaceTypeIter->second;
}

bool SaiSandiaPhyPlatform::isSerdesApiSupported() const {
  return true;
}
bool SaiSandiaPhyPlatform::supportInterfaceType() const {
  return false;
}
void SaiSandiaPhyPlatform::initLEDs() {
  throw FbossError("SaiSandiaPhyPlatform doesn't support initLEDs()");
}

sai_service_method_table_t* SaiSandiaPhyPlatform::getServiceMethodTable()
    const {
  return &kSaiServiceMethodTable;
}

const std::set<sai_api_t>& SaiSandiaPhyPlatform::getSupportedApiList() const {
  auto getApiList = [this]() {
    std::set<sai_api_t> apis(getDefaultPhyAsicSupportedApis());
    // Sandia SAI does not support ACL API so remove it
    apis.erase(facebook::fboss::AclApi::ApiType);
    return apis;
  };

  static const std::set<sai_api_t> kSupportedMacsecPhyApiList = getApiList();
  return kSupportedMacsecPhyApiList;
}

void SaiSandiaPhyPlatform::preHwInitialized() {
  // Call SaiSwitch::initSaiApis before creating SaiSwitch.
  // Only call this function once to make sure we only initialize sai apis
  // once even we'll create multiple SaiSwitch based on how many Marvell Phys
  // in the system.
  SaiApiTable::getInstance()->queryApis(
      getServiceMethodTable(), getSupportedApiList());
}

void SaiSandiaPhyPlatform::initSandiaSaiProfileValues() {
  kSaiProfileValues.insert(std::make_pair(
      folly::sformat("XPHY{:d}_{:s}", phyId_, SAI_KEY_INIT_CONFIG_FILE),
      getHwConfigDumpFile()));
  kSaiProfileValues.insert(std::make_pair(
      folly::sformat("XPHY{:d}_{:s}", phyId_, SAI_KEY_WARM_BOOT_READ_FILE),
      getWarmBootHelper()->warmBootDataPath()));
  kSaiProfileValues.insert(std::make_pair(
      folly::sformat("XPHY{:d}_{:s}", phyId_, SAI_KEY_WARM_BOOT_WRITE_FILE),
      getWarmBootHelper()->warmBootDataPath()));
  kSaiProfileValues.insert(std::make_pair(
      folly::sformat("XPHY{:d}_{:s}", phyId_, SAI_KEY_BOOT_TYPE),
      getWarmBootHelper()->canWarmBoot() ? "1" : "0"));
  kSaiProfileValues.insert(std::make_pair(
      folly::sformat("XPHY{:d}_{:s}", phyId_, SAI_KEY_BOOT_TYPE), "0"));

  for (auto& profile : kSaiProfileValues) {
    XLOG(DBG3) << "initSandiaSaiProfileValues: " << profile.first << " = "
               << profile.second;
  }
}

void SaiSandiaPhyPlatform::initImpl(uint32_t hwFeaturesDesired) {
  initSandiaSaiProfileValues();
  saiSwitch_ = std::make_unique<SaiSwitch>(this, hwFeaturesDesired);
}
} // namespace facebook::fboss
