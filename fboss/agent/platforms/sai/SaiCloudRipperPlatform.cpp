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
    folly::MacAddress localMac)
    : SaiTajoPlatform(
          std::move(productInfo),
          std::make_unique<CloudRipperPlatformMapping>(),
          localMac) {}

void SaiCloudRipperPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId) {
  asic_ = std::make_unique<EbroAsic>(switchType, switchId);
#if defined(TAJO_SDK_VERSION_1_56_0)
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
    folly::MacAddress localMac)
    : SaiCloudRipperPlatform(
          std::move(productInfo),
          std::make_unique<CloudRipperVoqPlatformMapping>(),
          localMac) {}

SaiCloudRipperFabricPlatform::SaiCloudRipperFabricPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiCloudRipperPlatform(
          std::move(productInfo),
          std::make_unique<CloudRipperFabricPlatformMapping>(),
          localMac) {}

} // namespace facebook::fboss
