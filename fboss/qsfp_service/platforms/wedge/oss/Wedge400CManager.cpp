// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/platforms/wedge/Wedge400CManager.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformUtil.h"
#include "fboss/lib/fpga/Wedge400TransceiverApi.h"

namespace facebook {
namespace fboss {

Wedge400CManager::Wedge400CManager()
    : WedgeManager(
          std::make_unique<Wedge400TransceiverApi>(),
          createWedge400CPlatformMapping(),
          PlatformType::PLATFORM_WEDGE400C) {}

std::unique_ptr<TransceiverI2CApi> Wedge400CManager::getI2CBus() {
  return std::make_unique<Wedge400I2CBus>();
}

std::unique_ptr<PlatformMapping>
Wedge400CManager::createWedge400CPlatformMapping() {
  return std::make_unique<Wedge400CPlatformMapping>();
}

} // namespace fboss
} // namespace facebook
