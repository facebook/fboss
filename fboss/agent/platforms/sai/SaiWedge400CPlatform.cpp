/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiWedge400CPlatform.h"
#include "fboss/agent/hw/sai/api/ArsApi.h"
#include "fboss/agent/hw/sai/api/ArsProfileApi.h"
#include "fboss/agent/hw/sai/api/UdfApi.h"
#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CGrandTetonPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformUtil.h"

#include <algorithm>

namespace facebook::fboss {

SaiWedge400CPlatform::SaiWedge400CPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiTajoPlatform(
          std::move(productInfo),
          createWedge400CPlatformMapping(platformMappingStr),
          localMac) {}

void SaiWedge400CPlatform::setupAsic(
    std::optional<int64_t> switchId,
    const cfg::SwitchInfo& switchInfo,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  std::optional<cfg::SdkVersion> sdkVersion;
#if defined(TAJO_SDK_GTE_24_8_3001)
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
    sdkVersion->asicSdk() = "24.8.3001";
  }
#endif
  asic_ = std::make_unique<EbroAsic>(switchId, switchInfo, sdkVersion);
}

HwAsic* SaiWedge400CPlatform::getAsic() const {
  return asic_.get();
}

SaiWedge400CPlatform::~SaiWedge400CPlatform() {}

std::unique_ptr<PlatformMapping>
SaiWedge400CPlatform::createWedge400CPlatformMapping(
    const std::string& platformMappingStr) {
  if (utility::isWedge400CPlatformRackTypeInference()) {
    return platformMappingStr.empty()
        ? std::make_unique<Wedge400CGrandTetonPlatformMapping>()
        : std::make_unique<Wedge400CGrandTetonPlatformMapping>(
              platformMappingStr);
  }
  return platformMappingStr.empty()
      ? std::make_unique<Wedge400CPlatformMapping>()
      : std::make_unique<Wedge400CPlatformMapping>(platformMappingStr);
}

const std::set<sai_api_t>& SaiWedge400CPlatform::getSupportedApiList() const {
  static auto apis = getDefaultSwitchAsicSupportedApis();
  apis.erase(facebook::fboss::UdfApi::ApiType);
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  apis.erase(facebook::fboss::ArsApi::ApiType);
  apis.erase(facebook::fboss::ArsProfileApi::ApiType);
#endif
  return apis;
}

} // namespace facebook::fboss
