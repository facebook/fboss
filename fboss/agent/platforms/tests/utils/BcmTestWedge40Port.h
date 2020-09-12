#pragma once

#include "fboss/agent/platforms/tests/utils/BcmTestPort.h"

namespace facebook::fboss {

class BcmTestWedge40Platform;

class BcmTestWedge40Port : public BcmTestPort {
 public:
  BcmTestWedge40Port(PortID id, BcmTestWedge40Platform* platform);

  LaneSpeeds supportedLaneSpeeds() const override {
    return {cfg::PortSpeed::GIGE, cfg::PortSpeed::XG};
  }

  void linkStatusChanged(bool up, bool adminUp) override;

 private:
  // Forbidden copy constructor and assignment operator
  BcmTestWedge40Port(BcmTestWedge40Port const&) = delete;
  BcmTestWedge40Port& operator=(BcmTestWedge40Port const&) = delete;
};

} // namespace facebook::fboss
