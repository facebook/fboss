// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwSwitch;

class HwTestPacketTrapEntry {
 public:
  HwTestPacketTrapEntry(HwSwitch* hwSwitch, PortID port);
  HwTestPacketTrapEntry(HwSwitch* hwSwitch, folly::CIDRNetwork& dstPrefix);
  ~HwTestPacketTrapEntry();

 private:
  int unit_;
  std::vector<int> entries_;
};

} // namespace facebook::fboss
