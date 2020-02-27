// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include "fboss/agent/hw/HwSwitchWarmBootHelper.h"

#include <folly/dynamic.h>

namespace facebook::fboss {

class BcmWarmBootHelper : public HwSwitchWarmBootHelper {
 public:
  BcmWarmBootHelper(int unit, const std::string& warmBootDir);

  void warmBootRead(uint8_t* buf, int offset, int nbytes);
  void warmBootWrite(const uint8_t* buf, int offset, int nbytes);

 private:
  // Forbidden copy constructor and assignment operator
  BcmWarmBootHelper(BcmWarmBootHelper const&) = delete;
  BcmWarmBootHelper& operator=(BcmWarmBootHelper const&) = delete;

  static int
  warmBootReadCallback(int unitNumber, uint8_t* buf, int offset, int nbytes);
  static int
  warmBootWriteCallback(int unitNumber, uint8_t* buf, int offset, int nbytes);
  void setupSdkWarmBoot();
};

} // namespace facebook::fboss
