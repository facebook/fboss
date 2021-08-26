// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwSwitch;

class HwTestPacketTrapEntry {
 public:
  HwTestPacketTrapEntry(HwSwitch* hwSwitch, const std::set<PortID>& ports);
  HwTestPacketTrapEntry(HwSwitch* hwSwitch, PortID port)
      : HwTestPacketTrapEntry(hwSwitch, std::set<PortID>({port})) {}
  HwTestPacketTrapEntry(
      HwSwitch* hwSwitch,
      const std::set<folly::CIDRNetwork>& dstPrefixes);
  HwTestPacketTrapEntry(HwSwitch* hwSwitch, const folly::CIDRNetwork& dstPrefix)
      : HwTestPacketTrapEntry(
            hwSwitch,
            std::set<folly::CIDRNetwork>({dstPrefix})) {}
  ~HwTestPacketTrapEntry();

 private:
  int unit_;
  std::vector<int> entries_;
};

} // namespace facebook::fboss
