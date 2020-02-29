/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/wedge/galaxy/GalaxyPlatform.h"

namespace facebook::fboss {

class WedgePortMapping;
class PlatformProductInfo;

class GalaxyLCPlatform : public GalaxyPlatform {
 public:
  explicit GalaxyLCPlatform(std::unique_ptr<PlatformProductInfo> productInfo);

  std::unique_ptr<WedgePortMapping> createPortMapping() override;

 private:
  GalaxyLCPlatform(GalaxyLCPlatform const&) = delete;
  GalaxyLCPlatform& operator=(GalaxyLCPlatform const&) = delete;
};

} // namespace facebook::fboss
