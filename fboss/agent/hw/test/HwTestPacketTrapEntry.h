// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {

class HwSwitch;
class AclEntry;

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
  HwSwitch* hwSwitch_;
  std::vector<int> entries_;
  std::vector<std::shared_ptr<AclEntry>> aclEntries_;
};

} // namespace facebook::fboss
