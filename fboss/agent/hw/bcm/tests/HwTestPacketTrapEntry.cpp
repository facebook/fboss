// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

extern "C" {
#include <bcm/field.h>
}

namespace facebook::fboss {

HwTestPacketTrapEntry::HwTestPacketTrapEntry(HwSwitch* hwSwitch, PortID port) {
  unit_ = static_cast<facebook::fboss::BcmSwitch*>(hwSwitch)->getUnit();
  const bcm_field_group_t gid =
      static_cast<const facebook::fboss::BcmSwitch*>(hwSwitch)
          ->getPlatform()
          ->getAsic()
          ->getDefaultACLGroupID();
  auto rv = bcm_field_entry_create(unit_, gid, &entry_) ||
      bcm_field_qualify_SrcPort(0, entry_, 0, 0xff, port, 0xff) ||
      bcm_field_action_add(0, entry_, bcmFieldActionCopyToCpu, 0, 0) ||
      bcm_field_entry_install(unit_, entry_);
  bcmCheckError(rv, "HwTestPacketTrapEntry creation failed");
}

HwTestPacketTrapEntry::~HwTestPacketTrapEntry() {
  bcm_field_entry_remove(unit_, entry_);
  bcm_field_action_delete(unit_, entry_, bcmFieldActionCopyToCpu, 0, 0);
  bcm_field_entry_destroy(unit_, entry_);
}

} // namespace facebook::fboss
