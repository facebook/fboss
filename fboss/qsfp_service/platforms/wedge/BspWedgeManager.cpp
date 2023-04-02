// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/platforms/wedge/BspWedgeManager.h"
#include <folly/gen/Base.h>
#include <folly/logging/xlog.h>
#include "fboss/lib/bsp/BspIOBus.h"
#include "fboss/lib/bsp/BspSystemContainer.h"

namespace facebook {
namespace fboss {

BspWedgeManager::BspWedgeManager(
    const BspSystemContainer* systemContainer,
    std::unique_ptr<BspTransceiverApi> api,
    std::unique_ptr<PlatformMapping> platformMapping,
    PlatformType type)
    : WedgeManager(std::move(api), std::move(platformMapping), type) {
  XLOG(INFO) << "BspTrace: BspWedgeManager()";
  systemContainer_ = systemContainer;
}

std::unique_ptr<TransceiverI2CApi> BspWedgeManager::getI2CBus() {
  XLOG(INFO) << "BspTrace: getI2CBus()";
  return std::make_unique<BspIOBus>(systemContainer_);
}

int BspWedgeManager::getNumQsfpModules() {
  return systemContainer_->getNumTransceivers();
}

} // namespace fboss
} // namespace facebook
