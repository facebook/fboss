#pragma once

#include "fboss/agent/platforms/test_platforms/BcmTestPort.h"

namespace facebook {
namespace fboss {
class BcmTestWedge40Platform;

class BcmTestWedge40Port : public BcmTestPort {
 public:
  BcmTestWedge40Port(PortID id, BcmTestWedge40Platform* platform);

  LaneSpeeds supportedLaneSpeeds() const override {
    return {cfg::PortSpeed::GIGE, cfg::PortSpeed::XG};
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedge40Port(BcmTestWedge40Port const&) = delete;
  BcmTestWedge40Port& operator=(BcmTestWedge40Port const&) = delete;
};
} // namespace fboss
} // namespace facebook
