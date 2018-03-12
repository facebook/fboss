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

#include "fboss/agent/platforms/wedge/WedgePlatform.h"

#include <folly/Range.h>
#include <memory>

namespace facebook { namespace fboss {

class BcmSwitch;
class MinipackPort;
class WedgeProductInfo;

class MinipackPlatform : public WedgePlatform {
public:
  explicit MinipackPlatform(std::unique_ptr<WedgeProductInfo> productInfo) :
    WedgePlatform(std::move(productInfo)) {}

  std::unique_ptr<WedgePortMapping> createPortMapping() override;

private:
  MinipackPlatform(MinipackPlatform const &) = delete;
  MinipackPlatform& operator=(MinipackPlatform const &) = delete;
};
}} // namespace faceboo::fboss
