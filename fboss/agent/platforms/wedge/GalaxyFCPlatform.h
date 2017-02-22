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

#include "fboss/agent/platforms/wedge/GalaxyPlatform.h"

namespace facebook { namespace fboss {

class WedgePortMapping;
class WedgeProductInfo;

class GalaxyFCPlatform : public GalaxyPlatform {
 public:
  explicit GalaxyFCPlatform(std::unique_ptr<WedgeProductInfo> productInfo) :
      GalaxyPlatform(std::move(productInfo)) {}

 private:
  std::unique_ptr<WedgePortMapping> createPortMapping() override;

  GalaxyFCPlatform(GalaxyFCPlatform const &) = delete;
  GalaxyFCPlatform& operator=(GalaxyFCPlatform const &) = delete;
};

}} // namespace facebook::fboss
