// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/platforms/wedge/Wedge400Manager.h"
#include "fboss/lib/fpga/Wedge400I2CBus.h"
#include "fboss/lib/fpga/Wedge400TransceiverApi.h"

namespace facebook::fboss {

Wedge400Manager::Wedge400Manager(
    const std::shared_ptr<const PlatformMapping> platformMapping,
    const std::shared_ptr<QsfpServiceThreads> qsfpServiceThreads)
    : WedgeManager(
          std::make_unique<Wedge400TransceiverApi>(),
          platformMapping,
          PlatformType::PLATFORM_WEDGE400,
          qsfpServiceThreads) {}

std::unique_ptr<TransceiverI2CApi> Wedge400Manager::getI2CBus() {
  return std::make_unique<Wedge400I2CBus>();
}

} // namespace facebook::fboss
