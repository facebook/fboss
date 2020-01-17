// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include <cstdint>

namespace facebook::fboss {

class SocUtils {
 public:
  static bool isTrident2(int unit);
  static bool isTomahawk(int unit);
  static bool isTomahawk3(int unit);

 private:
  // Forbidden copy constructor and assignment operator
  SocUtils(SocUtils const&) = delete;
  SocUtils& operator=(SocUtils const&) = delete;

  static uint16_t getDeviceId(int unit);
};

} // namespace facebook::fboss
