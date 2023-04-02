#include "fboss/qsfp_service/platforms/wedge/GalaxyManager.h"

#include "fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyLCPlatformMapping.h"
#include "fboss/lib/usb/GalaxyI2CBus.h"
#include "fboss/lib/usb/TransceiverPlatformI2cApi.h"
#include "fboss/qsfp_service/module/QsfpModule.h"

namespace facebook {
namespace fboss {

GalaxyManager::GalaxyManager(PlatformType mode)
    : WedgeManager(
          std::make_unique<TransceiverPlatformI2cApi>(new GalaxyI2CBus()),
          (mode == PlatformType::PLATFORM_GALAXY_LC)
              ? (std::unique_ptr<PlatformMapping>)
                    std::make_unique<GalaxyLCPlatformMapping>(
                        GalaxyLCPlatformMapping::getLinecardName())
              : (std::unique_ptr<PlatformMapping>)
                    std::make_unique<GalaxyFCPlatformMapping>(
                        GalaxyFCPlatformMapping::getFabriccardName()),
          mode) {}
// TODO: Will fully migrate I2CBusApi into TransceiverPlatformApi. Then we will
// construct the bus pointer before construct WedgeManager and will get rid of
// getI2CBus at that time.
std::unique_ptr<TransceiverI2CApi> GalaxyManager::getI2CBus() {
  return std::make_unique<WedgeI2CBusLock>(std::make_unique<GalaxyI2CBus>());
}
} // namespace fboss
} // namespace facebook
