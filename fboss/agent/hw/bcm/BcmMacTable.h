// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/state/MacEntry.h"
#include "fboss/agent/state/Vlan.h"

namespace facebook::fboss {

class BcmSwitch;

class BcmMacTable {
 public:
  explicit BcmMacTable(BcmSwitch* hw) : hw_(hw) {}
  ~BcmMacTable() {}

  void processMacAdded(const MacEntry* addedEntry, VlanID vlan);
  void processMacRemoved(const MacEntry* removedEntry, VlanID vlan);
  void processMacChanged(
      const MacEntry* oldEntry,
      const MacEntry* newEntry,
      VlanID vlan);

 private:
  void programMacEntry(const MacEntry* macEntry, VlanID vlan);
  void unprogramMacEntry(const MacEntry* macEntry, VlanID vlan);

  BcmSwitch* hw_;
};

} // namespace facebook::fboss
