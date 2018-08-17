#pragma once

#include "fboss/lib/usb/WedgeI2CBus.h"

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace facebook { namespace fboss {
class Wedge100Manager : public WedgeManager {
 public:
  Wedge100Manager();
  ~Wedge100Manager() override {}
  int getNumQsfpModules() override { return 32; }
 private:
  // Forbidden copy constructor and assignment operator
  Wedge100Manager(Wedge100Manager const &) = delete;
  Wedge100Manager& operator=(Wedge100Manager const &) = delete;
 protected:
  std::unique_ptr<TransceiverI2CApi> getI2CBus() override;
};
}} // facebook::fboss
