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

#include "fboss/agent/hw/switch_asics/EbroAsic.h"
#include "fboss/agent/platforms/common/cloud_ripper/CloudRipperFabricPlatformMapping.h"
#include "fboss/agent/platforms/common/cloud_ripper/CloudRipperPlatformMapping.h"
#include "fboss/agent/platforms/common/cloud_ripper/CloudRipperVoqPlatformMapping.h"

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
    std::optional<cfg::Range64> systemPortRange) {
  asic_ = std::make_unique<EbroAsic>(switchType, switchId, systemPortRange);
#if defined(TAJO_SDK_VERSION_1_58_0) || defined(TAJO_SDK_VERSION_1_62_0)
  asic_->setDefaultStreamType(cfg::StreamType::UNICAST);
#endif
}

std::string SaiCloudRipperPlatform::getHwConfig() {
  return *config()->thrift.platform()->get_chip().get_asic().config();
}

HwAsic* SaiCloudRipperPlatform::getAsic() const {
  return asic_.get();
}

SaiCloudRipperPlatform::~SaiCloudRipperPlatform() {}

SaiCloudRipperPlatform::SaiCloudRipperPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<CloudRipperVoqPlatformMapping> mapping,
    folly::MacAddress localMac)
    : SaiTajoPlatform(std::move(productInfo), std::move(mapping), localMac) {}

SaiCloudRipperPlatform::SaiCloudRipperPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<CloudRipperFabricPlatformMapping> mapping,
    folly::MacAddress localMac)
    : SaiTajoPlatform(std::move(productInfo), std::move(mapping), localMac) {}

SaiCloudRipperVoqPlatform::SaiCloudRipperVoqPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiCloudRipperPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<CloudRipperVoqPlatformMapping>()
              : std::make_unique<CloudRipperVoqPlatformMapping>(
                    platformMappingStr),
          localMac) {}

SaiCloudRipperFabricPlatform::SaiCloudRipperFabricPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr)
    : SaiCloudRipperPlatform(
          std::move(productInfo),
          platformMappingStr.empty()
              ? std::make_unique<CloudRipperFabricPlatformMapping>()
              : std::make_unique<CloudRipperFabricPlatformMapping>(
                    platformMappingStr),
          localMac) {}

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

} // namespace facebook::fboss
