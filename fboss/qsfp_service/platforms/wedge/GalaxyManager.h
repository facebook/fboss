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

#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/lib/usb/GalaxyI2CBus.h"
#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace facebook::fboss {

class GalaxyManager : public WedgeManager {
 public:
  explicit GalaxyManager(
      PlatformType type,
      const std::shared_ptr<const PlatformMapping> platformMapping,
      const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
          threads);
  ~GalaxyManager() override {}

  // This is the front panel ports count
  int getNumQsfpModules() const override {
    return 16;
  }

 private:
  // Forbidden copy constructor and assignment operator
  GalaxyManager(GalaxyManager const&) = delete;
  GalaxyManager& operator=(GalaxyManager const&) = delete;

  std::unique_ptr<TransceiverI2CApi> getI2CBus() override;
  GalaxyI2CBus i2cBus_;
};
} // namespace facebook::fboss
