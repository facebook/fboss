/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiPhyPlatform.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/Agera3PhyAsic.h"

DEFINE_string(
    phy_config_file,
    "/lib/firmware/fboss/phy_config",
    "Path to the PHY configuration file");

namespace facebook::fboss {

namespace {

std::unordered_map<std::string, std::string> kSaiProfileValues;

auto constexpr sdkWarmBootStateFile = "sdk_warm_boot_state";
static auto constexpr kSaiBootType = "SAI_BOOT_TYPE";
static auto constexpr kSaiInitConfigFile = "SAI_INIT_CONFIG_FILE";

static auto constexpr kSaiWarmBootReadFile = "SAI_WARM_BOOT_READ_FILE";
static auto constexpr kSaiWarmBootWriteFile = "SAI_WARM_BOOT_WRITE_FILE";

/*
 * saiProfileGetValue
 *
 * This function returns some key values to the SAI while doing
 * sai_api_initialize.
 */
const char* FOLLY_NULLABLE saiProfileGetValue(
    sai_switch_profile_id_t /*profileId*/,
    const char* variable) {
  auto saiProfileValItr = kSaiProfileValues.find(variable);
  if (saiProfileValItr != kSaiProfileValues.end()) {
    return saiProfileValItr->second.c_str();
  }
  return nullptr;
}

/*
 * saiProfileGetNextValue
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

SaiPhyPlatform::SaiPhyPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping,
    folly::MacAddress localMac,
    uint8_t pimId,
    int phyId)
    : SaiPlatform(std::move(productInfo), std::move(platformMapping), localMac),
      pimId_(pimId),
      phyId_(phyId),
      agentDirUtil_(new AgentDirectoryUtil(
          FLAGS_volatile_state_dir_phy + "/" + folly::to<std::string>(phyId_),
          FLAGS_persistent_state_dir_phy + "/" +
              folly::to<std::string>(phyId_))) {}

void SaiPhyPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  switch (*switchInfo.asicType()) {
    case cfg::AsicType::ASIC_TYPE_AGERA3:
      asic_ = std::make_unique<Agera3PhyAsic>(switchId, switchInfo);
      break;
    default:
      throw FbossError("Unknown asic type: ", *switchInfo.asicType());
  }
}

SaiPhyPlatform::~SaiPhyPlatform() {}

std::string SaiPhyPlatform::getHwConfig() {
  throw FbossError("SaiPhyPlatform doesn't support getHwConfig()");
}
HwAsic* SaiPhyPlatform::getAsic() const {
  return asic_.get();
}

std::vector<PortID> SaiPhyPlatform::getAllPortsInGroup(
    PortID /* portID */) const {
  throw FbossError("SaiPhyPlatform doesn't support FlexPort");
}
std::vector<FlexPortMode> SaiPhyPlatform::getSupportedFlexPortModes() const {
  throw FbossError("SaiPhyPlatform doesn't support FlexPort");
}
std::optional<sai_port_interface_type_t> SaiPhyPlatform::getInterfaceType(
    TransmitterTechnology /* transmitterTech */,
    cfg::PortSpeed /* speed */) const {
  throw FbossError("SaiPhyPlatform doesn't support getInterfaceType()");
}
bool SaiPhyPlatform::isSerdesApiSupported() const {
  return true;
}
bool SaiPhyPlatform::supportInterfaceType() const {
  return false;
}
void SaiPhyPlatform::initLEDs() {
  throw FbossError("SaiPhyPlatform doesn't support initLEDs()");
}

sai_service_method_table_t* SaiPhyPlatform::getServiceMethodTable() const {
  return &kSaiServiceMethodTable;
}

const std::set<sai_api_t>& SaiPhyPlatform::getSupportedApiList() const {
  return getDefaultPhyAsicSupportedApis();
}

void SaiPhyPlatform::initSaiProfileValues(bool warmboot) {
  auto sdkWarmBootStatePath =
      FLAGS_volatile_state_dir_phy + "/" + sdkWarmBootStateFile;

  kSaiProfileValues.insert(
      std::make_pair(kSaiInitConfigFile, FLAGS_phy_config_file));

  kSaiProfileValues.insert(
      std::make_pair(kSaiWarmBootReadFile, sdkWarmBootStatePath));

  kSaiProfileValues.insert(
      std::make_pair(kSaiWarmBootWriteFile, sdkWarmBootStatePath));

  kSaiProfileValues.insert(std::make_pair(kSaiBootType, warmboot ? "1" : "0"));
}

void SaiPhyPlatform::preHwInitialized(bool warmboot) {
  // Call SaiSwitch::initSaiApis before creating SaiSwitch.
  // Only call this function once to make sure we only initialize sai apis once
  SaiApiTable::getInstance()->queryApis(
      getServiceMethodTable(), getSupportedApiList());

  initSaiProfileValues(warmboot);
}

void SaiPhyPlatform::initImpl(uint32_t hwFeaturesDesired) {
  saiSwitch_ = std::make_unique<SaiSwitch>(this, hwFeaturesDesired);
}
} // namespace facebook::fboss
