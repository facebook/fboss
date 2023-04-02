#include "fboss/qsfp_service/platforms/wedge/Wedge100Manager.h"

#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"
#include "fboss/lib/usb/TransceiverPlatformI2cApi.h"
#include "fboss/lib/usb/Wedge100I2CBus.h"

namespace facebook {
namespace fboss {
Wedge100Manager::Wedge100Manager()
    : WedgeManager(
          std::make_unique<TransceiverPlatformI2cApi>(new Wedge100I2CBus()),
          std::make_unique<Wedge100PlatformMapping>(),
          PlatformType::PLATFORM_WEDGE100) {}
// TODO: Will fully migrate I2CBusApi into TransceiverPlatformApi. Then we will
// construct the bus pointer before construct WedgeManager and will get rid of
// getI2CBus at that time.

std::unique_ptr<TransceiverI2CApi> Wedge100Manager::getI2CBus() {
  return std::make_unique<WedgeI2CBusLock>(std::make_unique<Wedge100I2CBus>());
}
} // namespace fboss
} // namespace facebook
