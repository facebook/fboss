// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/platforms/wedge/Wedge400CManager.h"
#include "fboss/lib/fpga/Wedge400TransceiverApi.h"

namespace facebook::fboss {

Wedge400CManager::Wedge400CManager(
    const std::shared_ptr<const PlatformMapping> platformMapping)
    : WedgeManager(
          std::make_unique<Wedge400TransceiverApi>(),
          platformMapping,
          PlatformType::PLATFORM_WEDGE400C) {}

std::unique_ptr<TransceiverI2CApi> Wedge400CManager::getI2CBus() {
  return std::make_unique<Wedge400I2CBus>();
}

} // namespace facebook::fboss
