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

#include "fboss/agent/platforms/sai/SaiTajoPlatform.h"

namespace facebook::fboss {

class Wedge400CEbbLabPlatformMapping;

class SaiWedge400CPlatform : public SaiTajoPlatform {
 public:
  explicit SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
  ~SaiWedge400CPlatform() override;
  std::string getHwConfig() override;

 protected:
  SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<Wedge400CEbbLabPlatformMapping> mapping,
      folly::MacAddress localMac);
};

class SaiWedge400CEbbLabPlatform : public SaiWedge400CPlatform {
 public:
  explicit SaiWedge400CEbbLabPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
};

} // namespace facebook::fboss
