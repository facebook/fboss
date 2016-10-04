#pragma once

#include "fboss/lib/usb/WedgeI2CBus.h"

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace facebook { namespace fboss {
class Wedge40Manager : public WedgeManager {
 public:
  Wedge40Manager();
  ~Wedge40Manager() override {}
 private:
  // Forbidden copy constructor and assignment operator
  Wedge40Manager(Wedge40Manager const &) = delete;
  Wedge40Manager& operator=(Wedge40Manager const &) = delete;
};
}} // facebook::fboss
