// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "fboss/agent/types.h"

#include <folly/IPAddressV6.h>

namespace facebook::fboss {

class HwSwitch;

class HwTestPacketTrapEntry {
 public:
  HwTestPacketTrapEntry(HwSwitch* hwSwitch, PortID port);
  HwTestPacketTrapEntry(HwSwitch* hwSwitch, folly::IPAddress& dstIp);
  ~HwTestPacketTrapEntry();

 private:
  int unit_;
  int entry_;
};

} // namespace facebook::fboss
