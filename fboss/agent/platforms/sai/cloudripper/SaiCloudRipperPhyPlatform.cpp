/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/cloudripper/SaiCloudRipperPhyPlatform.h"

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/CredoPhyAsic.h"
#include "fboss/agent/platforms/common/cloud_ripper/CloudRipperPlatformMapping.h"

namespace facebook::fboss {

const std::string& SaiCloudRipperPhyPlatform::getFirmwareDirectory() {
  static const std::string kFirmwareDir = "/lib/firmware/fboss/credo/f104/";
  return kFirmwareDir;
}

namespace {
static auto constexpr kSaiBootType = "SAI_KEY_BOOT_TYPE";
static auto constexpr kSaiConfigFile = "SAI_KEY_INIT_CONFIG_FILE";

const std::string kPhyConfigProfile =
    SaiCloudRipperPhyPlatform::getFirmwareDirectory() + "Cloud_Ripper.xml";

/*
 * saiProfileGetValue
 *
 * This function returns some key values to the SAI while doing
 * sai_api_initialize.
 * For SAI_KEY_BOOT_TYPE, currently we only return the cold boot type.
 * For SAI_KEY_INIT_CONFIG_FILE, the profile id tells SAI which default
 * configuration to pick up for the Phy.
 */
const char* FOLLY_NULLABLE saiProfileGetValue(
    sai_switch_profile_id_t /*profileId*/,
    const char* variable) {
  if (strcmp(variable, kSaiBootType) == 0) {
    // TODO(vsp) Support warmboot
    return "cold";
  } else if (strcmp(variable, kSaiConfigFile) == 0) {
    return kPhyConfigProfile.c_str();
  }
  return nullptr;
}

/*
 * saiProfileGetNextValue
 *
 * This function lets SAI pick up next value for a given key. Currently this
 * returns null
 */
int saiProfileGetNextValue(
    sai_switch_profile_id_t /* profile_id */,
    const char** /* variable */,
    const char** /* value */) {
  return -1;
}

sai_service_method_table_t kSaiServiceMethodTable = {
    .profile_get_value = saiProfileGetValue,
    .profile_get_next_value = saiProfileGetNextValue,
};
} // namespace

SaiCloudRipperPhyPlatform::SaiCloudRipperPhyPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    int phyId)
    : SaiPlatform(
          std::move(productInfo),
          std::make_unique<CloudRipperPlatformMapping>(),
          localMac),
      phyId_(phyId),
      agentDirUtil_(new AgentDirectoryUtil(
          FLAGS_volatile_state_dir_phy + "/" + folly::to<std::string>(phyId_),
          FLAGS_persistent_state_dir_phy + "/" +
              folly::to<std::string>(phyId_))) {}

void SaiCloudRipperPhyPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<CredoPhyAsic>(
      switchType, switchId, switchIndex, systemPortRange, mac);
}

SaiCloudRipperPhyPlatform::~SaiCloudRipperPhyPlatform() {}

std::string SaiCloudRipperPhyPlatform::getHwConfig() {
  throw FbossError("SaiCloudRipperPhyPlatform doesn't support getHwConfig()");
}
HwAsic* SaiCloudRipperPhyPlatform::getAsic() const {
  return asic_.get();
}

std::vector<PortID> SaiCloudRipperPhyPlatform::getAllPortsInGroup(
    PortID /* portID */) const {
  throw FbossError("SaiCloudRipperPhyPlatform doesn't support FlexPort");
}
std::vector<FlexPortMode> SaiCloudRipperPhyPlatform::getSupportedFlexPortModes()
    const {
  throw FbossError("SaiCloudRipperPhyPlatform doesn't support FlexPort");
}
std::optional<sai_port_interface_type_t>
SaiCloudRipperPhyPlatform::getInterfaceType(
    TransmitterTechnology /* transmitterTech */,
    cfg::PortSpeed /* speed */) const {
  throw FbossError(
      "SaiCloudRipperPhyPlatform doesn't support getInterfaceType()");
}
bool SaiCloudRipperPhyPlatform::isSerdesApiSupported() const {
  return true;
}
bool SaiCloudRipperPhyPlatform::supportInterfaceType() const {
  return false;
}
void SaiCloudRipperPhyPlatform::initLEDs() {
  throw FbossError("SaiCloudRipperPhyPlatform doesn't support initLEDs()");
}

sai_service_method_table_t* SaiCloudRipperPhyPlatform::getServiceMethodTable()
    const {
  return &kSaiServiceMethodTable;
}

const std::set<sai_api_t>& SaiCloudRipperPhyPlatform::getSupportedApiList()
    const {
  auto getApiList = [this]() {
    std::set<sai_api_t> apis(getDefaultPhyAsicSupportedApis());
    apis.insert(facebook::fboss::MacsecApi::ApiType);
    return apis;
  };

  static const std::set<sai_api_t> kSupportedMacsecPhyApiList = getApiList();
  return kSupportedMacsecPhyApiList;
}

void SaiCloudRipperPhyPlatform::preHwInitialized() {
  // Call SaiSwitch::initSaiApis before creating SaiSwitch.
  // Only call this function once to make sure we only initialize sai apis
  // once even we'll create multiple SaiSwitch based on how many Cloud Ripper
  // PHYs in the system.
  SaiApiTable::getInstance()->queryApis(
      getServiceMethodTable(), getSupportedApiList());
}

void SaiCloudRipperPhyPlatform::initImpl(uint32_t hwFeaturesDesired) {
  saiSwitch_ = std::make_unique<SaiSwitch>(this, hwFeaturesDesired);
}
} // namespace facebook::fboss
