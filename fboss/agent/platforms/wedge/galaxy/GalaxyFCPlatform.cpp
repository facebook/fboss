/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/galaxy/GalaxyFCPlatform.h"

#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.h"
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
#include "fboss/agent/platforms/wedge/galaxy/GalaxyPort.h"

namespace facebook::fboss {

GalaxyFCPlatform::GalaxyFCPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : GalaxyPlatform(
          std::move(productInfo),
          std::make_unique<GalaxyFCPlatformMapping>(
              GalaxyFCPlatformMapping::getFabriccardName())) {}

std::unique_ptr<WedgePortMapping> GalaxyFCPlatform::createPortMapping() {
  return WedgePortMapping::createFromConfig<
      WedgePortMappingT<GalaxyPlatform, GalaxyPort>>(this);
}

} // namespace facebook::fboss
