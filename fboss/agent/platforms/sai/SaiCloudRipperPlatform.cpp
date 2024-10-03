/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiCloudRipperPlatform.h"

#include "fboss/agent/hw/sai/api/ArsApi.h"
#include "fboss/agent/hw/sai/api/ArsProfileApi.h"
#include "fboss/agent/hw/sai/api/UdfApi.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/platforms/common/cloud_ripper/CloudRipperPlatformMapping.h"

namespace facebook::fboss {

SaiCloudRipperPlatform::SaiCloudRipperPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiTajoPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<CloudRipperPlatformMapping>()
              : std::make_unique<CloudRipperPlatformMapping>(
                    platformMappingStr),
          localMac) {}

void SaiCloudRipperPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  std::optional<cfg::SdkVersion> sdkVersion;
#if defined(TAJO_SDK_GTE_24_4_90)
  /*
   * HwAsic table instance in the sw switch reads the SDK version
   * from the agent config for prod and from sai switch ensemble
   * for hw test. However, hw asic instance owned by the sai switch
   * do not carry the SDK version. Hence, populating the SDK version
   */
  auto agentConfig = config();
  if (agentConfig->thrift.sw()->sdkVersion().has_value()) {
    sdkVersion = agentConfig->thrift.sw()->sdkVersion().value();
  } else {
    sdkVersion = cfg::SdkVersion{};
    sdkVersion->asicSdk() = "24.4.90";
  }
#endif
  asic_ = std::make_unique<EbroAsic>(
      switchType, switchId, switchIndex, systemPortRange, mac, sdkVersion);
}

HwAsic* SaiCloudRipperPlatform::getAsic() const {
  return asic_.get();
}

SaiCloudRipperPlatform::~SaiCloudRipperPlatform() = default;

std::vector<sai_system_port_config_t>
SaiCloudRipperPlatform::getInternalSystemPortConfig() const {
  CHECK(asic_) << " Asic must be set before getting sys port info";
  CHECK(asic_->getSwitchId()) << " Switch Id must be set before sys port info";
  auto switchIdVal = static_cast<uint32_t>(*asic_->getSwitchId());
  // Below are a mixture system port configs for
  // internal {loopback, CPU} ports. From the speeds (in Mbps)
  // one can infer that ports 6-10 are 1G CPU ports and 10-14 are
  // 100G loopback ports (TODO - confirm this with vendor)
  //
  return {
      {11, switchIdVal, 0, 25, 100000, 8},
      {12, switchIdVal, 2, 25, 100000, 8},
      {13, switchIdVal, 4, 25, 100000, 8},
      {14, switchIdVal, 6, 25, 100000, 8},
      {15, switchIdVal, 8, 25, 100000, 8},
      {16, switchIdVal, 10, 25, 100000, 8},
      {6, switchIdVal, 0, 24, 1000, 8},
      {7, switchIdVal, 4, 24, 1000, 8},
      {8, switchIdVal, 6, 24, 1000, 8},
      {9, switchIdVal, 8, 24, 1000, 8},
      {10, switchIdVal, 1, 24, 1000, 8}};
}

const std::set<sai_api_t>& SaiCloudRipperPlatform::getSupportedApiList() const {
  static auto apis = getDefaultSwitchAsicSupportedApis();
  apis.erase(facebook::fboss::UdfApi::ApiType);
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  apis.erase(facebook::fboss::ArsApi::ApiType);
  apis.erase(facebook::fboss::ArsProfileApi::ApiType);
#endif
  return apis;
}

} // namespace facebook::fboss
