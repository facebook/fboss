// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <array>
#include <string>

namespace facebook::fboss::platform::helpers {
class ScdHelper {
 public:
  ScdHelper();
  void disable_wdt();
  void kick_wdt(uint32_t);
  virtual ~ScdHelper();

 private:
  uint32_t readReg(uint32_t regAddr);
  void writeReg(uint32_t regAddr, uint32_t val);
  int fd_{0};
  volatile uint8_t* mapAddr_;
  uint32_t mapSize_;
};
} // namespace facebook::fboss::platform::helpers
