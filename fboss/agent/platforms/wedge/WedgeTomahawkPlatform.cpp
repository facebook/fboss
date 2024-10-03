/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/WedgeTomahawkPlatform.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {

WedgeTomahawkPlatform::WedgeTomahawkPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping,
    folly::MacAddress localMac)
    : WedgePlatform(
          std::move(productInfo),
          std::move(platformMapping),
          localMac) {}

void WedgeTomahawkPlatform::setupAsic(
    cfg::SwitchType switchType,
    std::optional<int64_t> switchId,
    int16_t switchIndex,
    std::optional<cfg::Range64> systemPortRange,
    folly::MacAddress& mac,
    std::optional<HwAsic::FabricNodeRole> fabricNodeRole) {
  CHECK(!fabricNodeRole.has_value());
  asic_ = std::make_unique<TomahawkAsic>(
      switchType, switchId, switchIndex, systemPortRange, mac);
}

const PortQueue& WedgeTomahawkPlatform::getDefaultPortQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultPortQueueSettings(
      utility::BcmChip::TOMAHAWK, streamType);
}

const PortQueue& WedgeTomahawkPlatform::getDefaultControlPlaneQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultControlPlaneQueueSettings(
      utility::BcmChip::TOMAHAWK, streamType);
}

} // namespace facebook::fboss
