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
#include <algorithm>
#include "fboss/agent/hw/switch_asics/TajoAsic.h"
#include "fboss/agent/platforms/common/cloud_ripper/CloudRipperPlatformMapping.h"

namespace facebook::fboss {

SaiCloudRipperPlatform::SaiCloudRipperPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac)
    : SaiTajoPlatform(
          std::move(productInfo),
          std::make_unique<CloudRipperPlatformMapping>(),
          localMac) {}

std::string SaiCloudRipperPlatform::getHwConfig() {
  return *config()->thrift.platform_ref()->get_chip().get_asic().config_ref();
}

SaiCloudRipperPlatform::~SaiCloudRipperPlatform() {}

} // namespace facebook::fboss
