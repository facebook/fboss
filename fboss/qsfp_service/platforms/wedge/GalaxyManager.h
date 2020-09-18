#pragma once

#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace facebook { namespace fboss {
class GalaxyManager : public WedgeManager {
 public:
  explicit GalaxyManager(PlatformMode mode);
  ~GalaxyManager() override {}

  // This is the front panel ports count
  int getNumQsfpModules() override { return 16; }
 private:
  // Forbidden copy constructor and assignment operator
  GalaxyManager(GalaxyManager const &) = delete;
  GalaxyManager& operator=(GalaxyManager const &) = delete;
 protected:
  std::unique_ptr<TransceiverI2CApi> getI2CBus() override;
};
}} // facebook::fboss
