#include "fboss/qsfp_service/platforms/wedge/Wedge40Manager.h"
#include "fboss/lib/usb/TransceiverPlatformI2cApi.h"

namespace facebook::fboss {

Wedge40Manager::Wedge40Manager(
    const std::shared_ptr<const PlatformMapping> platformMapping)
    : WedgeManager(
          std::make_unique<TransceiverPlatformI2cApi>(new WedgeI2CBus()),
          platformMapping,
          PlatformType::PLATFORM_WEDGE) {}
// TODO: Will fully migrate I2CBusApi into TransceiverPlatformApi. Then we will
// construct the bus pointer before construct WedgeManager and will get rid of
// getI2CBus at that time.
} // namespace facebook::fboss
