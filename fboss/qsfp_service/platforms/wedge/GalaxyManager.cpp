#include "fboss/qsfp_service/platforms/wedge/GalaxyManager.h"

#include "fboss/lib/usb/GalaxyI2CBus.h"
#include "fboss/qsfp_service/sff/QsfpModule.h"

namespace facebook { namespace fboss {
GalaxyManager::GalaxyManager(){
}

std::unique_ptr<BaseWedgeI2CBus> GalaxyManager::getI2CBus() {
  return std::make_unique<GalaxyI2CBus>();
}
}} // facebook::fboss
