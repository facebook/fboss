// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/platforms/wedge/Wedge400CManager.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformMapping.h"
#include "fboss/agent/platforms/common/wedge400c/Wedge400CPlatformUtil.h"
#include "fboss/lib/fpga/Wedge400TransceiverApi.h"

namespace facebook {
namespace fboss {

Wedge400CManager::Wedge400CManager(const std::string& platformMappingStr)
    : WedgeManager(
          std::make_unique<Wedge400TransceiverApi>(),
          createWedge400CPlatformMapping(platformMappingStr),
          PlatformType::PLATFORM_WEDGE400C) {}

std::unique_ptr<TransceiverI2CApi> Wedge400CManager::getI2CBus() {
  return std::make_unique<Wedge400I2CBus>();
}

const std::shared_ptr<const PlatformMapping>
Wedge400CManager::createWedge400CPlatformMapping(
    const std::string& platformMappingStr) {
  return platformMappingStr.empty()
      ? std::make_shared<Wedge400CPlatformMapping>()
      : std::make_shared<Wedge400CPlatformMapping>(platformMappingStr);
}

} // namespace fboss
} // namespace facebook
