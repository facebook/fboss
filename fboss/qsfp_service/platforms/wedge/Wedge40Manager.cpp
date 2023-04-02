#include "fboss/qsfp_service/platforms/wedge/Wedge40Manager.h"

#include "fboss/agent/platforms/common/wedge40/Wedge40PlatformMapping.h"
#include "fboss/lib/usb/TransceiverPlatformI2cApi.h"

namespace facebook {
namespace fboss {
Wedge40Manager::Wedge40Manager()
    : WedgeManager(
          std::make_unique<TransceiverPlatformI2cApi>(new WedgeI2CBus()),
          std::make_unique<Wedge40PlatformMapping>(),
          PlatformType::PLATFORM_WEDGE) {}
// TODO: Will fully migrate I2CBusApi into TransceiverPlatformApi. Then we will
// construct the bus pointer before construct WedgeManager and will get rid of
// getI2CBus at that time.
} // namespace fboss
} // namespace facebook
