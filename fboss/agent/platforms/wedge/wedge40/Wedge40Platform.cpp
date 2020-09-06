/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/wedge40/Wedge40Platform.h"

#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/common/wedge40/Wedge40PlatformMapping.h"
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
#include "fboss/agent/platforms/wedge/wedge40/Wedge40Port.h"

#include "fboss/agent/platforms/common/utils/Wedge40LedUtils.h"

#include <folly/Range.h>

namespace facebook::fboss {

Wedge40Platform::Wedge40Platform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : WedgePlatform(
          std::move(productInfo),
          std::make_unique<Wedge40PlatformMapping>()) {
  asic_ = std::make_unique<Trident2Asic>();
}

std::unique_ptr<WedgePortMapping> Wedge40Platform::createPortMapping() {
  return WedgePortMapping::createFromConfig<
      WedgePortMappingT<Wedge40Platform, Wedge40Port>>(this);
}

folly::ByteRange Wedge40Platform::defaultLed0Code() {
  return Wedge40LedUtils::defaultLedCode();
}

folly::ByteRange Wedge40Platform::defaultLed1Code() {
  return defaultLed0Code();
}

const PortQueue& Wedge40Platform::getDefaultPortQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultPortQueueSettings(
      utility::BcmChip::TRIDENT2, streamType);
}

const PortQueue& Wedge40Platform::getDefaultControlPlaneQueueSettings(
    cfg::StreamType streamType) const {
  return utility::getDefaultControlPlaneQueueSettings(
      utility::BcmChip::TRIDENT2, streamType);
}

} // namespace facebook::fboss
