// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "fboss/agent/types.h"

namespace facebook::fboss {

class HwSwitch;

class HwTestPacketTrapEntry {
 public:
  HwTestPacketTrapEntry(HwSwitch* hwSwitch, PortID port);
  ~HwTestPacketTrapEntry();

 private:
  int unit_;
  int entry_;
};

} // namespace facebook::fboss
