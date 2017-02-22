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

#include "fboss/agent/platforms/wedge/Wedge40Platform.h"

namespace facebook { namespace fboss {

class WedgePortMapping;
class WedgeProductInfo;

class SixpackFCPlatform : public Wedge40Platform {
 public:
  explicit SixpackFCPlatform(std::unique_ptr<WedgeProductInfo> productInfo) :
      Wedge40Platform(std::move(productInfo)) {}

  std::unique_ptr<WedgePortMapping> createPortMapping() override;

 private:
  SixpackFCPlatform(SixpackFCPlatform const &) = delete;
  SixpackFCPlatform& operator=(SixpackFCPlatform const &) = delete;
};

}} // namespace facebook::fboss
