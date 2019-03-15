#pragma once

#include "fboss/agent/hw/bcm/tests/platforms/BcmTestPort.h"

namespace facebook {
namespace fboss {
class FakeBcmTestPort : public BcmTestPort {
 public:
  explicit FakeBcmTestPort(PortID id);

  LaneSpeeds supportedLaneSpeeds() const override {
    return {cfg::PortSpeed::GIGE,
            cfg::PortSpeed::XG,
            cfg::PortSpeed::TWENTYFIVEG};
  }

 private:
  // Forbidden copy constructor and assignment operator
  FakeBcmTestPort(FakeBcmTestPort const&) = delete;
  FakeBcmTestPort& operator=(FakeBcmTestPort const&) = delete;
};
} // namespace fboss
} // namespace facebook
