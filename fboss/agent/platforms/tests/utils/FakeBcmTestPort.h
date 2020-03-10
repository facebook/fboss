#pragma once

#include "fboss/agent/platforms/tests/utils/BcmTestPort.h"

namespace facebook::fboss {

class FakeBcmTestPlatform;

class FakeBcmTestPort : public BcmTestPort {
 public:
  FakeBcmTestPort(PortID id, FakeBcmTestPlatform* platform);

  LaneSpeeds supportedLaneSpeeds() const override {
    return {
        cfg::PortSpeed::GIGE, cfg::PortSpeed::XG, cfg::PortSpeed::TWENTYFIVEG};
  }

 private:
  // Forbidden copy constructor and assignment operator
  FakeBcmTestPort(FakeBcmTestPort const&) = delete;
  FakeBcmTestPort& operator=(FakeBcmTestPort const&) = delete;
};

} // namespace facebook::fboss
