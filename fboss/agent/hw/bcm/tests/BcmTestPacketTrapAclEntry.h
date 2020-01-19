// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once
#include "fboss/agent/types.h"

extern "C" {
#include <bcm/field.h>
}

namespace facebook::fboss {

class BcmTestPacketTrapAclEntry {
 public:
  BcmTestPacketTrapAclEntry(int unit, PortID port);
  ~BcmTestPacketTrapAclEntry();

 private:
  int unit_;
  bcm_field_entry_t entry_;
};

} // namespace facebook::fboss
