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

#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
namespace facebook::fboss {

class SaiBcmWedge100Platform : public SaiBcmPlatform {
 public:
  using SaiBcmPlatform::SaiBcmPlatform;
  std::vector<PortID> masterLogicalPortIds() const override;
};

} // namespace facebook::fboss
